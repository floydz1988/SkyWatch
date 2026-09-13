// ============================================================
//  opensky.cpp — OpenSky Network REST client
// ============================================================
#include "opensky.h"
#include "haversine.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

uint32_t g_last_fetch_ms = 0;

// Build bounding-box URL from home coords + radius
static String buildURL() {
  // Add generous margin: radius_km / 111 deg ≈ 1° per 111 km
  float margin = g_config.radius_km / 111.0f + 0.05f;
  float lamin   = g_config.lat - margin;
  float lamax   = g_config.lat + margin;
  float lomin   = g_config.lon - margin;
  float lomax   = g_config.lon + margin;

  char url[200];
  snprintf(url, sizeof(url),
    "http://opensky-network.org/api/states/all"
    "?lamin=%.4f&lomin=%.4f&lamax=%.4f&lomax=%.4f",
    lamin, lomin, lamax, lomax);
  return String(url);
}

// ── Parse one state vector row ────────────────────────────
// OpenSky states/all column indices:
//  0  icao24   (string)
//  1  callsign (string)
//  5  longitude (float or null)
//  6  latitude  (float or null)
//  7  baro_altitude (float or null, metres)
//  8  on_ground (bool)
//  9  velocity  (float or null, m/s)
// 10  true_track / heading (float or null, degrees)
static bool parseState(JsonArray& row, Flight& f) {
  if (row.size() < 11) return false;

  // icao24
  if (row[0].isNull()) return false;
  strlcpy(f.icao24, row[0].as<const char*>() ?: "", sizeof(f.icao24));

  // callsign (may have trailing spaces)
  const char* cs = row[1].isNull() ? "" : row[1].as<const char*>();
  strlcpy(f.callsign, cs, sizeof(f.callsign));
  // Trim trailing whitespace
  for (int i = strlen(f.callsign) - 1; i >= 0 && f.callsign[i] == ' '; i--)
    f.callsign[i] = '\0';

  // Position — skip if null (not receiving position)
  if (row[5].isNull() || row[6].isNull()) return false;
  f.lon = row[5].as<float>();
  f.lat = row[6].as<float>();

  f.altitude_m = row[7].isNull() ? 0.0f : row[7].as<float>();
  f.on_ground  = row[8].as<bool>();
  float vel_ms = row[9].isNull()  ? 0.0f : row[9].as<float>();
  f.speed_kmh  = vel_ms * 3.6f;
  f.heading_deg = row[10].isNull() ? 0.0f : row[10].as<float>();

  return true;
}

// ── Main fetch ────────────────────────────────────────────
int openskyFetch() {
  if (!g_wifi_connected) return -1;

  HTTPClient http;
  http.begin(buildURL());
  http.setTimeout(8000);
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return -1;
  }

  // Stream parse — the payload can be 50–200 KB
  // Use a DynamicJsonDocument sized generously;
  // on ESP32-S3 with ~512 KB PSRAM this is fine.
  // Without PSRAM, limit MAX_FLIGHTS and use filter.
  WiFiClient* stream = http.getStreamPtr();

  // Build filter to keep only the columns we need
  // (reduces RAM usage significantly)
  StaticJsonDocument<200> filter;
  JsonArray statesFilter = filter.createNestedArray("states");
  JsonArray rowFilter    = statesFilter.createNestedArray();
  rowFilter.add(true);  // 0 icao24
  rowFilter.add(true);  // 1 callsign
  rowFilter.add(false); // 2 origin_country
  rowFilter.add(false); // 3 time_position
  rowFilter.add(false); // 4 last_contact
  rowFilter.add(true);  // 5 longitude
  rowFilter.add(true);  // 6 latitude
  rowFilter.add(true);  // 7 baro_altitude
  rowFilter.add(true);  // 8 on_ground
  rowFilter.add(true);  // 9 velocity
  rowFilter.add(true);  // 10 true_track

  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, *stream,
                               DeserializationOption::Filter(filter));
  http.end();

  if (err != DeserializationError::Ok) return -1;

  JsonArray states = doc["states"];
  if (states.isNull()) {
    g_flight_count = 0;
    return 0;
  }

  g_flight_count  = 0;
  g_closest_idx   = -1;
  float closestDist = 9999.0f;

  for (JsonArray row : states) {
    if (g_flight_count >= MAX_FLIGHTS) break;

    Flight f;
    memset(&f, 0, sizeof(f));
    if (!parseState(row, f)) continue;

    // Distance filter
    f.distance_km = haversine(g_config.lat, g_config.lon, f.lat, f.lon);
    if (f.distance_km > g_config.radius_km) continue;

    // ETA (only meaningful if not on ground)
    if (!f.on_ground && f.speed_kmh > 10.0f) {
      f.eta_seconds = calcETA(f.lat, f.lon,
                               g_config.lat, g_config.lon,
                               f.speed_kmh, f.heading_deg);
    } else {
      f.eta_seconds = -1;
    }

    g_flights[g_flight_count] = f;

    if (f.distance_km < closestDist) {
      closestDist   = f.distance_km;
      g_closest_idx = g_flight_count;
    }
    g_flight_count++;
  }

  g_last_fetch_ms = millis();
  return g_flight_count;
}
