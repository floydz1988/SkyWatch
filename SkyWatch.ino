// ============================================================
//  SkyWatch.ino — Main sketch
//  Waveshare ESP32-S3-Knob-Touch-LCD-1.8
//  Live ADS-B aircraft tracker with config portal
// ============================================================
//  Libraries required (install via Arduino Library Manager):
//    • LovyanGFX          (display driver)
//    • ArduinoJson        (JSON parsing)
//    • ESP32 Arduino core (built-in: WiFi, WebServer, LittleFS,
//                          HTTPClient, Preferences)
// ============================================================

#include <WiFi.h>
#include "config.h"
#include "display.h"
#include "knob.h"
#include "wifi_manager.h"
#include "flight.h"
#include "haversine.h"
#include "opensky.h"
#include "screens.h"
#include "aircraft_photo.h"
#include "screensaver.h"
#include "records.h"

// ── State machine ─────────────────────────────────────────
static ScreenState _state       = STATE_SCREENSAVER;
static ScreenState _prevState   = STATE_SCREENSAVER;

// ── Night-dim tracking ────────────────────────────────────
static uint32_t _lastActivityMs = 0;
static bool     _dimmed         = false;

static void wakeUp() {
  _lastActivityMs = millis();
  if (_dimmed) {
    displayBrightness(BL_FULL);
    _dimmed = false;
  }
}

// ── State transitions ─────────────────────────────────────
static void enterState(ScreenState s) {
  _prevState = _state;
  _state     = s;
  wakeUp();

  switch (s) {
    case STATE_SCREENSAVER:
      screensaverDraw();
      break;

    case STATE_RADAR:
      drawRadarScreen();
      break;

    case STATE_ALERT:
      drawAlertScreen(g_closest_idx);
      break;

    case STATE_LIST:
      drawListScreen(g_selected_idx);
      break;

    case STATE_DETAIL:
      drawDetailScreen(g_selected_idx);
      // Fetch photo asynchronously (blocks briefly, ~2-5s on good WiFi)
      if (g_selected_idx >= 0 && g_selected_idx < g_flight_count) {
        fetchAndDrawPhoto(g_flights[g_selected_idx].icao24);
      }
      break;
  }
}

// ── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== SkyWatch boot ===");

  displayInit();
  knobInit();

  // Show branding while we load config + connect
  screensaverDraw();

  recordsLoad();
  wifiLoadConfig();

  // Try to connect to saved WiFi
  if (wifiConnect()) {
    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi not connected — portal available on long-press");
  }

  _lastActivityMs = millis();
}

// ── Portal mode loop ───────────────────────────────────────
static void runPortalLoop() {
  // Draw portal active screen
  tft.fillScreen(C_BG);
  drawTopBar("SETUP PORTAL", false);
  tft.setTextSize(1);
  tft.setTextColor(C_GREEN);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Connect to WiFi:", SCREEN_W/2, 60);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("SkyWatch-Setup", SCREEN_W/2, 80);
  tft.setTextColor(C_MUTED);
  tft.drawString("Password: skywatch1", SCREEN_W/2, 100);
  tft.drawString("Then open:", SCREEN_W/2, 124);
  tft.setTextColor(C_ORANGE);
  tft.drawString("192.168.4.1", SCREEN_W/2, 144);
  tft.setTextColor(C_DIM);
  tft.drawString("Device restarts after save.", SCREEN_W/2, 180);

  while (true) {
    wifiHandlePortal();
    delay(5);
  }
}

// ── Main loop ─────────────────────────────────────────────
void loop() {
  // Portal takes over if active
  if (wifiPortalActive()) {
    runPortalLoop();
    return;   // never reached; ESP.restart() inside handleSave()
  }

  knobUpdate();

  // ── Night-dim ──────────────────────────────────────────
  if (!_dimmed && (millis() - _lastActivityMs > DIM_AFTER_MS)) {
    displayBrightness(BL_DIM);
    _dimmed = true;
  }

  // ── Long press anywhere → portal (or back) ─────────────
  if (knobLongPressed()) {
    wakeUp();
    if (_state == STATE_DETAIL || _state == STATE_ALERT || _state == STATE_LIST) {
      enterState(STATE_RADAR);
    } else if (_state == STATE_RADAR || _state == STATE_SCREENSAVER) {
      // Long press on radar/screensaver → launch config portal
      wifiStartSetupPortal();
      runPortalLoop();
    }
    return;
  }

  // ── Per-state logic ────────────────────────────────────
  switch (_state) {

    // ── SCREENSAVER ────────────────────────────────────────
    case STATE_SCREENSAVER: {
      screensaverTick();
      if (knobPressed() || knobDelta() != 0) {
        enterState(STATE_RADAR);
      }
      break;
    }

    // ── RADAR ──────────────────────────────────────────────
    case STATE_RADAR: {
      // Animate sweep
      tickRadar();

      // Knob rotate → go to list
      int8_t d = knobDelta();
      if (d != 0) {
        wakeUp();
        enterState(STATE_LIST);
        break;
      }

      // Knob press → list (or alert if something close)
      if (knobPressed()) {
        wakeUp();
        if (g_closest_idx >= 0 &&
            g_flights[g_closest_idx].distance_km <= g_config.alert_dist_km) {
          g_selected_idx = g_closest_idx;
          enterState(STATE_ALERT);
        } else {
          g_selected_idx = (g_closest_idx >= 0) ? g_closest_idx : 0;
          enterState(STATE_LIST);
        }
        break;
      }

      // Periodic API fetch
      if (millis() - g_last_fetch_ms >= OPENSKY_INTERVAL_MS) {
        int n = openskyFetch();
        Serial.printf("OpenSky: %d flights\n", n);
        // Update records for every new flight seen
        for (int i = 0; i < g_flight_count; i++) {
          recordsUpdate(g_flights[i]);
        }
        // Auto-alert if something very close
        if (g_closest_idx >= 0 &&
            g_flights[g_closest_idx].distance_km <= g_config.alert_dist_km) {
          g_selected_idx = g_closest_idx;
          enterState(STATE_ALERT);
        } else {
          // Refresh radar footer with new flight count
          drawRadarScreen();
        }
      }
      break;
    }

    // ── ALERT ──────────────────────────────────────────────
    case STATE_ALERT: {
      // Update ETA once per second
      static uint32_t _lastEtaMs = 0;
      if (millis() - _lastEtaMs >= 1000) {
        _lastEtaMs = millis();
        // Recalculate ETA live
        if (g_selected_idx >= 0 && g_selected_idx < g_flight_count) {
          Flight& f = g_flights[g_selected_idx];
          if (!f.on_ground && f.speed_kmh > 10.0f) {
            f.eta_seconds = calcETA(f.lat, f.lon,
                                    g_config.lat, g_config.lon,
                                    f.speed_kmh, f.heading_deg);
            // Crude position extrapolation
            float dt   = 1.0f / 3600.0f;   // 1 s in hours
            float dKm  = f.speed_kmh * dt;
            float rad  = (f.heading_deg - 90.0f) * DEG_TO_RAD;
            f.lon += cosf(rad) * (dKm / (111.0f * cosf(f.lat * DEG_TO_RAD)));
            f.lat += sinf(rad) * (dKm / 111.0f);
            f.distance_km = haversine(g_config.lat, g_config.lon, f.lat, f.lon);
          }
          updateAlertETA(g_selected_idx);
        }
      }

      if (knobPressed()) {
        wakeUp();
        enterState(STATE_DETAIL);
        break;
      }

      int8_t d2 = knobDelta();
      if (d2 != 0) {
        wakeUp();
        enterState(STATE_LIST);
        break;
      }

      // Full API refresh
      if (millis() - g_last_fetch_ms >= OPENSKY_INTERVAL_MS) {
        openskyFetch();
        // Re-draw alert with fresh data
        if (g_selected_idx < g_flight_count) {
          drawAlertScreen(g_selected_idx);
        } else {
          enterState(STATE_RADAR);
        }
      }
      break;
    }

    // ── LIST ───────────────────────────────────────────────
    case STATE_LIST: {
      int8_t d3 = knobDelta();
      if (d3 != 0) {
        wakeUp();
        g_selected_idx = constrain(g_selected_idx + d3, 0, g_flight_count - 1);
        drawListScreen(g_selected_idx);
        break;
      }

      if (knobPressed()) {
        wakeUp();
        if (g_flight_count > 0) enterState(STATE_DETAIL);
        break;
      }

      // Periodic refresh
      if (millis() - g_last_fetch_ms >= OPENSKY_INTERVAL_MS) {
        openskyFetch();
        g_selected_idx = constrain(g_selected_idx, 0, max(0, g_flight_count - 1));
        drawListScreen(g_selected_idx);
        // Auto-alert
        if (g_closest_idx >= 0 &&
            g_flights[g_closest_idx].distance_km <= g_config.alert_dist_km) {
          g_selected_idx = g_closest_idx;
          enterState(STATE_ALERT);
        }
      }
      break;
    }

    // ── DETAIL ─────────────────────────────────────────────
    case STATE_DETAIL: {
      if (knobPressed()) {
        wakeUp();
        enterState(STATE_LIST);
        break;
      }
      int8_t d4 = knobDelta();
      if (d4 != 0) {
        wakeUp();
        g_selected_idx = constrain(g_selected_idx + d4, 0, g_flight_count - 1);
        enterState(STATE_DETAIL);
        break;
      }
      break;
    }
  }

  delay(10);  // Yield to WiFi stack
}
