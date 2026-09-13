#pragma once
// ============================================================
//  opensky.h — OpenSky Network API client
//  Fetches live ADS-B state vectors for a bounding box,
//  filters by haversine distance, populates g_flights[].
// ============================================================
#include <Arduino.h>
#include "flight.h"

// Returns number of flights found within scan radius, -1 on error.
int  openskyFetch();

// Last fetch timestamp (millis)
extern uint32_t g_last_fetch_ms;
