// ============================================================
//  records.cpp — NVS-backed personal flight records
// ============================================================
#include "records.h"

#if FEATURE_RECORDS
#include <Preferences.h>

PersonalRecords g_records;

static Preferences _prefs;
static const char* NS = "skywatch";   // NVS namespace

// ── Load ─────────────────────────────────────────────────
void recordsLoad() {
  _prefs.begin(NS, true);   // read-only

  g_records.totalFlightsSeen   = _prefs.getUInt("total",    0);
  g_records.closestApproachKm  = _prefs.getFloat("closeKm", 9999.0f);
  g_records.fastestSpeedKmh    = _prefs.getFloat("fastKmh", 0.0f);
  g_records.highestAltitudeM   = _prefs.getFloat("highM",   0.0f);

  String cs = _prefs.getString("closeCS", "");
  strlcpy(g_records.closestCallsign, cs.c_str(), sizeof(g_records.closestCallsign));

  String fs = _prefs.getString("fastCS",  "");
  strlcpy(g_records.fastestCallsign, fs.c_str(), sizeof(g_records.fastestCallsign));

  String hs = _prefs.getString("highCS",  "");
  strlcpy(g_records.highestCallsign, hs.c_str(), sizeof(g_records.highestCallsign));

  _prefs.end();
}

// ── Save (internal) ───────────────────────────────────────
void recordsSave() {
  _prefs.begin(NS, false);  // read-write

  _prefs.putUInt("total",   g_records.totalFlightsSeen);
  _prefs.putFloat("closeKm", g_records.closestApproachKm);
  _prefs.putFloat("fastKmh", g_records.fastestSpeedKmh);
  _prefs.putFloat("highM",   g_records.highestAltitudeM);
  _prefs.putString("closeCS", g_records.closestCallsign);
  _prefs.putString("fastCS",  g_records.fastestCallsign);
  _prefs.putString("highCS",  g_records.highestCallsign);

  _prefs.end();
}

// ── Update after each API fetch ────────────────────────────
void recordsUpdate(const Flight& f) {
  bool changed = false;

  g_records.totalFlightsSeen++;
  changed = true;

  if (f.distance_km > 0 && f.distance_km < g_records.closestApproachKm) {
    g_records.closestApproachKm = f.distance_km;
    strlcpy(g_records.closestCallsign, f.callsign, sizeof(g_records.closestCallsign));
    changed = true;
  }

  if (f.speed_kmh > g_records.fastestSpeedKmh) {
    g_records.fastestSpeedKmh = f.speed_kmh;
    strlcpy(g_records.fastestCallsign, f.callsign, sizeof(g_records.fastestCallsign));
    changed = true;
  }

  if (f.altitude_m > g_records.highestAltitudeM) {
    g_records.highestAltitudeM = f.altitude_m;
    strlcpy(g_records.highestCallsign, f.callsign, sizeof(g_records.highestCallsign));
    changed = true;
  }

  if (changed) recordsSave();
}

// ── Reset ─────────────────────────────────────────────────
void recordsReset() {
  memset(&g_records, 0, sizeof(g_records));
  g_records.closestApproachKm = 9999.0f;
  recordsSave();
}

#else // !FEATURE_RECORDS
// Personal records disabled — see FEATURE_RECORDS in config.h.
// Stubs keep every call site working unchanged (setup()/loop() in
// SkyWatch.ino) while reclaiming the Preferences/NVS code and the
// PersonalRecords global for other features.
void recordsLoad()                  {}
void recordsUpdate(const Flight&)   {}
void recordsSave()                  {}
void recordsReset()                 {}

#endif // FEATURE_RECORDS
