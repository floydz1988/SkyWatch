#pragma once
// ============================================================
//  aircraft_photo.h — Fetch + display real aircraft photo
//  Uses Planespotters.net public API (no auth needed):
//  GET https://api.planespotters.net/pub/photos/hex/{icao24}
//  Then fetches the thumbnail URL and decodes JPEG via LovyanGFX
// ============================================================
#include <Arduino.h>

// Fetch photo for the given ICAO24 hex and draw it into the
// detail screen photo region (x=0, y=20, w=SCREEN_W, h=130).
// Falls back to a vector silhouette if fetch fails.
// Call AFTER drawDetailScreen() so the grid is already drawn.
void fetchAndDrawPhoto(const char* icao24);
