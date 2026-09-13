#pragma once
// ============================================================
//  flight.h — Flight data structure
// ============================================================
#include <Arduino.h>
#include "config.h"

struct Flight {
  char    callsign[10];   // e.g. "DLH421"
  char    icao24[8];      // e.g. "3c6444"  (hex transponder code)
  float   lat;
  float   lon;
  float   altitude_m;
  float   speed_kmh;
  float   heading_deg;
  float   distance_km;    // from home, filled by opensky module
  int32_t eta_seconds;    // seconds until closest approach (-1 = unknown)
  bool    on_ground;
};

// Shared flight list (populated by opensky module)
extern Flight   g_flights[MAX_FLIGHTS];
extern int      g_flight_count;
extern int      g_selected_idx;   // highlighted row in list screen
extern int      g_closest_idx;    // index of nearest aircraft
