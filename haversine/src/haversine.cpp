// ============================================================
//  haversine.cpp — Great-circle distance + ETA math
// ============================================================
#include "haversine.h"
#include <Arduino.h>   // radians() / degrees() macros

float haversine(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371.0f;
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  float a = sinf(dLat / 2) * sinf(dLat / 2)
          + cosf(radians(lat1)) * cosf(radians(lat2))
          * sinf(dLon / 2) * sinf(dLon / 2);
  float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
  return R * c;
}

int32_t calcETA(float planeLat, float planeLon,
                 float homeLat,  float homeLon,
                 float speed_kmh, float heading_deg) {
  if (speed_kmh < 10.0f) return -1;

  // Bearing from plane to home
  float dLon = radians(homeLon - planeLon);
  float y = sinf(dLon) * cosf(radians(homeLat));
  float x = cosf(radians(planeLat)) * sinf(radians(homeLat))
           - sinf(radians(planeLat)) * cosf(radians(homeLat)) * cosf(dLon);
  float bearingToHome = degrees(atan2f(y, x));
  if (bearingToHome < 0) bearingToHome += 360.0f;

  // Angle between plane's heading and direction to home
  float diff = fabsf(heading_deg - bearingToHome);
  if (diff > 180.0f) diff = 360.0f - diff;

  if (diff > 90.0f) return -1;  // moving away

  float dist_km    = haversine(planeLat, planeLon, homeLat, homeLon);
  float approach_speed = speed_kmh * cosf(radians(diff));
  if (approach_speed < 1.0f) return -1;

  return (int32_t)((dist_km / approach_speed) * 3600.0f);  // seconds
}
