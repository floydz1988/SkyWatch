// ============================================================
//  aircraft_photo.cpp — Planespotters.net photo fetch + render
// ============================================================
#include "aircraft_photo.h"
#include "display.h"
#include "config.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Photo region on detail screen
#define PHOTO_X  0
#define PHOTO_Y  20
#define PHOTO_W  SCREEN_W   // 240
#define PHOTO_H  130

// ── SVG silhouette fallback (generic airliner in radar-green) ─
static void drawSilhouette() {
  tft.fillRect(PHOTO_X, PHOTO_Y, PHOTO_W, PHOTO_H, tft.color565(8, 8, 16));

  uint16_t col = C_GREEN;
  int cx = PHOTO_X + PHOTO_W / 2;
  int cy = PHOTO_Y + PHOTO_H / 2;

  // Fuselage (elongated rectangle)
  tft.fillRoundRect(cx - 60, cy - 6, 120, 12, 6, col);

  // Wings (swept trapezoid approximation with lines)
  for (int i = 0; i < 4; i++) {
    int thick = 3 - i;
    tft.drawLine(cx - 10 + i, cy,     cx - 55 + i, cy - 28, col);
    tft.drawLine(cx - 10 + i, cy,     cx - 55 + i, cy + 28, col);
    tft.drawLine(cx + 10 + i, cy,     cx + 40 + i, cy - 16, col);
    tft.drawLine(cx + 10 + i, cy,     cx + 40 + i, cy + 16, col);
    (void)thick;
  }

  // Wing fill boxes
  tft.fillTriangle(cx - 10, cy - 1,  cx - 55, cy - 28,  cx - 55, cy - 20, col);
  tft.fillTriangle(cx - 10, cy + 1,  cx - 55, cy + 28,  cx - 55, cy + 20, col);
  tft.fillTriangle(cx + 10, cy - 1,  cx + 40, cy - 16,  cx + 40, cy - 10, col);
  tft.fillTriangle(cx + 10, cy + 1,  cx + 40, cy + 16,  cx + 40, cy + 10, col);

  // Nose cone
  tft.fillCircle(cx + 60, cy, 6, col);

  // Vertical stabilizer
  tft.fillTriangle(cx - 55, cy - 6,  cx - 55, cy - 24,  cx - 40, cy - 6, col);

  // Label
  tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::bottom_center);
  tft.setTextSize(1);
  tft.drawString("No photo available", cx, PHOTO_Y + PHOTO_H - 4);
}

// ── Resolve thumbnail URL from Planespotters JSON ─────────
static bool resolvePhotoUrl(const char* icao24, char* photoUrl, size_t len) {
  char apiUrl[80];
  snprintf(apiUrl, sizeof(apiUrl),
    "https://api.planespotters.net/pub/photos/hex/%s", icao24);

  HTTPClient http;
  http.begin(apiUrl);
  http.setTimeout(6000);
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "SkyWatch/1.0 ESP32");

  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, *http.getStreamPtr());
  http.end();
  if (err != DeserializationError::Ok) return false;

  // Planespotters response: {"photos":[{"thumbnail_large":{"src":"..."},...},...]}
  JsonArray photos = doc["photos"];
  if (photos.isNull() || photos.size() == 0) return false;

  const char* src = photos[0]["thumbnail_large"]["src"];
  if (!src) src = photos[0]["thumbnail"]["src"];
  if (!src) return false;

  strlcpy(photoUrl, src, len);
  return true;
}

// ── Fetch JPEG bytes and draw with LovyanGFX ─────────────
static bool fetchAndDrawJpeg(const char* url) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);

  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  int jpegLen = http.getSize();
  if (jpegLen <= 0 || jpegLen > 80000) { http.end(); return false; }

  uint8_t* buf = (uint8_t*)malloc(jpegLen);
  if (!buf) { http.end(); return false; }

  WiFiClient* stream = http.getStreamPtr();
  int received = 0;
  while (received < jpegLen) {
    int avail = stream->available();
    if (avail <= 0) { delay(5); continue; }
    int chunk = stream->readBytes(buf + received,
                                  min(avail, jpegLen - received));
    received += chunk;
  }
  http.end();

  if (received != jpegLen) { free(buf); return false; }

  // Draw JPEG centred in photo region
  tft.fillRect(PHOTO_X, PHOTO_Y, PHOTO_W, PHOTO_H, tft.color565(0,0,0));
  tft.drawJpg(buf, jpegLen, PHOTO_X, PHOTO_Y, PHOTO_W, PHOTO_H,
              0, 0, lgfx::datum_t::middle_center);

  free(buf);
  return true;
}

// ── Public entry point ────────────────────────────────────
void fetchAndDrawPhoto(const char* icao24) {
  if (!g_wifi_connected || !icao24 || strlen(icao24) == 0) {
    drawSilhouette();
    return;
  }

  char photoUrl[256] = {};
  if (!resolvePhotoUrl(icao24, photoUrl, sizeof(photoUrl))) {
    drawSilhouette();
    return;
  }

  if (!fetchAndDrawJpeg(photoUrl)) {
    drawSilhouette();
  }
}
