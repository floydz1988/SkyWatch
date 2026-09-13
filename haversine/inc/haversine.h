#pragma once
// ============================================================
//  haversine.h — Great-circle distance + ETA math
// ============================================================
#include <math.h>
#include <stdint.h>

// Returns distance in km between two lat/lon points
float haversine(float lat1, float lon1, float lat2, float lon2);

// Estimate seconds until closest approach.
// Uses current distance, speed, and heading relative to home.
// Returns -1 if plane is moving away.
int32_t calcETA(float planeLat, float planeLon,
                 float homeLat,  float homeLon,
                 float speed_kmh, float heading_deg);
