#pragma once
// ============================================================
//  records.h — Personal flight records (stored in NVS)
//  Survives power-off and firmware updates.
//  Accessible from the Detail screen (long-hold, future menu).
// ============================================================
#include <Arduino.h>
#include "flight.h"
#include "config.h"

#if FEATURE_RECORDS
struct PersonalRecords {
  uint32_t totalFlightsSeen;      // all-time count
  float    closestApproachKm;     // smallest distance_km ever logged
  char     closestCallsign[10];
  float    fastestSpeedKmh;
  char     fastestCallsign[10];
  float    highestAltitudeM;
  char     highestCallsign[10];
};

extern PersonalRecords g_records;
#endif // FEATURE_RECORDS

// Always declared so call sites don't need to change when the feature is
// toggled — these become no-ops when FEATURE_RECORDS is 0.
void  recordsLoad();                      // Call once in setup()
void  recordsUpdate(const Flight& f);     // Call after each successful fetch
void  recordsSave();                      // Persist to NVS (called internally by recordsUpdate)
void  recordsReset();                     // Wipe all records (future menu option)
