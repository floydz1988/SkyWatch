// ============================================================
//  screensaver.cpp — Screensaver: custom logo or built-in brand
// ============================================================
#include "screensaver.h"
#include "display.h"
#include "config.h"
#include "wifi_manager.h"
#include <LittleFS.h>

static bool   _hasLogo     = false;
static uint8_t _pulse      = 0;
static bool   _pulseUp     = true;
static uint32_t _lastPulse = 0;

// ── Try to draw /logo.jpg from LittleFS ──────────────────
static bool tryDrawLogo() {
  if (!LittleFS.begin(false)) return false;
  if (!LittleFS.exists("/logo.jpg")) return false;

  File f = LittleFS.open("/logo.jpg", "r");
  if (!f) return false;

  size_t sz = f.size();
  if (sz == 0 || sz > 200000) { f.close(); return false; }

  uint8_t* buf = (uint8_t*)malloc(sz);
  if (!buf) { f.close(); return false; }

  size_t read = f.read(buf, sz);
  f.close();

  if (read != sz) { free(buf); return false; }

  tft.fillScreen(TFT_BLACK);
  // Centre the logo in the full screen (240×280)
  tft.drawJpg(buf, sz, 0, 0, SCREEN_W, SCREEN_H,
              0, 0, lgfx::datum_t::middle_center);
  free(buf);
  return true;
}

// ── Built-in branding fallback ─────────────────────────
static void drawBranding() {
  tft.fillScreen(C_BG);

  // Large ✈ icon
  tft.setTextColor(C_GREEN);
  tft.setTextSize(4);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString("SkyWatch", SCREEN_W / 2, SCREEN_H / 2 - 30);

  tft.setTextSize(1);
  tft.setTextColor(C_MUTED);
  tft.drawString("Live Aircraft Tracker", SCREEN_W / 2, SCREEN_H / 2 + 10);

  // Location / owner strip
  tft.setTextColor(C_DIM);
  char buf[48];
  snprintf(buf, sizeof(buf), "%s  ·  %s",
           g_config.owner_name, g_config.location_name);
  tft.drawString(buf, SCREEN_W / 2, SCREEN_H / 2 + 26);

  // Hint
  tft.setTextColor(C_DIM);
  tft.drawString("Press or rotate to start", SCREEN_W / 2, SCREEN_H - 16);
}

// ── Public API ────────────────────────────────────────────
void screensaverDraw() {
  _hasLogo = tryDrawLogo();
  if (!_hasLogo) {
    drawBranding();
  }
  _pulse    = 128;
  _pulseUp  = true;
  _lastPulse = millis();
}

void screensaverTick() {
  if (_hasLogo) return;   // Static logo — nothing to animate

  // Slowly pulse the hint text brightness
  uint32_t now = millis();
  if (now - _lastPulse < 30) return;
  _lastPulse = now;

  // Update pulse value
  if (_pulseUp) {
    _pulse += 3;
    if (_pulse >= 220) { _pulse = 220; _pulseUp = false; }
  } else {
    if (_pulse <= 30) { _pulse = 30; _pulseUp = true; }
    else _pulse -= 3;
  }

  uint16_t col = tft.color565(_pulse >> 2, _pulse >> 2, _pulse >> 2);
  tft.setTextColor(col);
  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(1);
  // Erase previous hint then redraw
  tft.fillRect(0, SCREEN_H - 24, SCREEN_W, 16, C_BG);
  tft.drawString("Press or rotate to start", SCREEN_W / 2, SCREEN_H - 16);
}
