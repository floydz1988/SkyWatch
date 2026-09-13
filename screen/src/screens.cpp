// ============================================================
//  screens.cpp — UI screen renderers
// ============================================================
#include "screens.h"
#include "display.h"
#include "flight.h"
#include "airlines.h"
#include "config.h"
#include "wifi_manager.h"
#include "haversine.h"
#include <math.h>

// ── Shared helpers ────────────────────────────────────────

static void drawDivider(int y) {
  tft.drawFastHLine(0, y, SCREEN_W, C_BORDER);
}

// Format seconds into "4m 32s" or "arriving"
static void fmtETA(int32_t secs, char* buf, size_t len) {
  if (secs < 0)       { snprintf(buf, len, "Away"); return; }
  if (secs < 60)      { snprintf(buf, len, "%ds",   (int)secs); return; }
  if (secs < 3600)    { snprintf(buf, len, "%dm%ds", (int)(secs/60), (int)(secs%60)); return; }
  snprintf(buf, len, "%dh%dm", (int)(secs/3600), (int)((secs%3600)/60));
}

// Format altitude in metres → feet string
static void fmtAlt(float m, char* buf, size_t len) {
  if (m <= 0) { snprintf(buf, len, "--"); return; }
  snprintf(buf, len, "%dft", (int)(m * 3.28084f));
}

// Draw a 2-cell data row: label left, value right
static void drawDataRow(int y, const char* label, const char* value, uint16_t valColor) {
  tft.setTextSize(1);
  tft.setTextDatum(lgfx::middle_left);
  tft.setTextColor(C_MUTED);
  tft.drawString(label, 8, y);
  tft.setTextDatum(lgfx::middle_right);
  tft.setTextColor(valColor);
  tft.drawString(value, SCREEN_W - 8, y);
}

// ── 1. RADAR SCREEN ───────────────────────────────────────
// Animated sweep arm + blip dots for each tracked flight.
// Layout: top bar 20px | radar circle centered in the remaining
// ~270px above the footer | stats footer ~85px

#define RADAR_CX     (SCREEN_W / 2)        // 180
#define RADAR_CY     155                   // centers the circle in the 20..~270 band
#define RADAR_R      110

static float  _sweepAngle = 0.0f;
static uint32_t _lastTickMs = 0;

// Trail fade table — blips decay after sweep passes them
struct BlipState { int idx; uint32_t seenAt; };
static BlipState _blips[MAX_FLIGHTS];
static int       _blipCount = 0;

static void radarToScreen(float angleDeg, float distKm, int* px, int* py) {
  float rad = (angleDeg - 90.0f) * DEG_TO_RAD;  // 0° = up
  float norm = distKm / g_config.radius_km;
  if (norm > 1.0f) norm = 1.0f;
  *px = RADAR_CX + (int)(cosf(rad) * norm * RADAR_R);
  *py = RADAR_CY + (int)(sinf(rad) * norm * RADAR_R);
}

// Draw bearing from home to flight
static float bearingTo(int idx) {
  float dlat = g_flights[idx].lat - g_config.lat;
  float dlon = g_flights[idx].lon - g_config.lon;
  float angle = atan2f(dlon, dlat) * RAD_TO_DEG;
  if (angle < 0) angle += 360.0f;
  return angle;
}

static void drawRadarBg() {
  tft.fillScreen(C_BG);
  drawTopBar("SKYWATCH RADAR", g_wifi_connected);

  // Radar rings
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R,     C_BORDER);
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R*2/3, C_BORDER);
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R/3,   C_BORDER);

  // Cross-hair
  tft.drawFastHLine(RADAR_CX - RADAR_R, RADAR_CY, RADAR_R * 2, C_BORDER);
  tft.drawFastVLine(RADAR_CX, RADAR_CY - RADAR_R, RADAR_R * 2, C_BORDER);

  // Home dot
  tft.fillCircle(RADAR_CX, RADAR_CY, 3, C_GREEN);

  // Radius labels
  tft.setTextSize(1); tft.setTextColor(C_DIM);
  char buf[8];
  snprintf(buf, sizeof(buf), "%.0fkm", g_config.radius_km / 3.0f);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(buf, RADAR_CX + RADAR_R/3 + 4, RADAR_CY - 4);
  snprintf(buf, sizeof(buf), "%.0fkm", g_config.radius_km * 2.0f / 3.0f);
  tft.drawString(buf, RADAR_CX + RADAR_R*2/3 + 4, RADAR_CY - 4);
}

static void drawRadarFooter() {
  int fy = RADAR_CY + RADAR_R + 8;
  drawDivider(fy - 4);

  tft.setTextSize(1);
  char buf[32];

  // Count
  tft.setTextColor(C_MUTED); tft.setTextDatum(lgfx::middle_left);
  snprintf(buf, sizeof(buf), "%d flights / %.0fkm", g_flight_count, g_config.radius_km);
  tft.drawString(buf, 8, fy + 10);

  // Location
  tft.setTextDatum(lgfx::middle_right); tft.setTextColor(C_GREEN);
  tft.drawString(g_config.location_name, SCREEN_W - 8, fy + 10);

  // Closest
  if (g_closest_idx >= 0) {
    tft.setTextColor(C_MUTED); tft.setTextDatum(lgfx::middle_left);
    snprintf(buf, sizeof(buf), "Closest: %.1fkm", g_flights[g_closest_idx].distance_km);
    tft.drawString(buf, 8, fy + 26);

    tft.setTextDatum(lgfx::middle_right); tft.setTextColor(C_ORANGE);
    tft.drawString(g_flights[g_closest_idx].callsign, SCREEN_W - 8, fy + 26);
  }

  // Hint
  tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Turn knob · Press for list", SCREEN_W/2, fy + 46);
}

void drawRadarScreen() {
  _sweepAngle = 0;
  _blipCount  = 0;
  _lastTickMs = millis();

  drawRadarBg();
  drawRadarFooter();
}

bool tickRadar() {
  uint32_t now = millis();
  float dt     = (now - _lastTickMs) / 1000.0f;
  _lastTickMs  = now;

  const float SWEEP_DEG_PER_S = 60.0f;   // one full rotation per 6 s
  _sweepAngle += SWEEP_DEG_PER_S * dt;
  if (_sweepAngle >= 360.0f) _sweepAngle -= 360.0f;

  // Erase old sweep (draw over in bg color)
  float prevAngle = _sweepAngle - SWEEP_DEG_PER_S * dt - 2.0f;
  if (prevAngle < 0) prevAngle += 360.0f;

  // Draw sweep wedge: thin trailing fade
  for (int fade = 0; fade < 20; fade++) {
    float a = _sweepAngle - fade * 0.8f;
    if (a < 0) a += 360.0f;
    float rad = (a - 90.0f) * DEG_TO_RAD;
    uint16_t col = (fade == 0) ? C_GREEN :
                   (fade < 5)  ? tft.color565(0, 80-fade*12, 40-fade*6) :
                                  C_BG;
    tft.drawLine(RADAR_CX, RADAR_CY,
                 RADAR_CX + (int)(cosf(rad) * RADAR_R),
                 RADAR_CY + (int)(sinf(rad) * RADAR_R), col);
  }

  // Re-draw rings (sweep erased them)
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R,     C_BORDER);
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R*2/3, C_BORDER);
  tft.drawCircle(RADAR_CX, RADAR_CY, RADAR_R/3,   C_BORDER);
  tft.fillCircle(RADAR_CX, RADAR_CY, 3, C_GREEN);

  // Light up blips near the sweep line
  for (int i = 0; i < g_flight_count; i++) {
    float bearing = bearingTo(i);
    float diff    = fabsf(bearing - _sweepAngle);
    if (diff > 180) diff = 360.0f - diff;
    if (diff < 4.0f) {
      // Sweep is passing this flight
      int px, py;
      radarToScreen(bearing, g_flights[i].distance_km, &px, &py);
      uint16_t blipColor = (g_flights[i].distance_km <= g_config.alert_dist_km)
                           ? C_RED : (g_flights[i].distance_km <= 4.0f ? C_ORANGE : C_GREEN);
      tft.fillCircle(px, py, 3, blipColor);
    }
  }

  return true;
}

// ── 2. ALERT SCREEN ──────────────────────────────────────
// Big airline badge + route + 6-cell grid + ETA countdown

void drawAlertScreen(int idx) {
  if (idx < 0 || idx >= g_flight_count) return;
  const Flight& f = g_flights[idx];

  tft.fillScreen(C_BG);
  drawTopBar("⚠ OVERHEAD", g_wifi_connected);

  // Pulsing red border effect
  tft.drawRect(1, 1, SCREEN_W-2, SCREEN_H-2, C_RED);
  tft.drawRect(2, 2, SCREEN_W-4, SCREEN_H-4, tft.color565(80, 0, 0));

  // Airline badge
  String iata      = extractIATA(f.callsign);
  AirlineInfo info = findAirline(iata);
  drawBadge(SCREEN_W/2 - 40, 40, 80, 34, info.color, iata.c_str(), TFT_WHITE, 6);

  // Callsign
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(f.callsign, SCREEN_W/2, 92);

  // Airline name
  tft.setTextSize(1); tft.setTextColor(C_MUTED);
  tft.drawString(info.name, SCREEN_W/2, 114);

  drawDivider(128);

  // 6-cell grid (2 columns × 3 rows)
  struct Cell { const char* label; char value[20]; uint16_t color; };
  Cell cells[6];

  snprintf(cells[0].value, 20, "%.1fkm",    f.distance_km);  cells[0].label = "DIST";    cells[0].color = C_RED;
  char etaBuf[12]; fmtETA(f.eta_seconds, etaBuf, sizeof(etaBuf));
  snprintf(cells[1].value, 20, "%s",  etaBuf);                cells[1].label = "ETA";     cells[1].color = C_ORANGE;
  char altBuf[12]; fmtAlt(f.altitude_m, altBuf, sizeof(altBuf));
  snprintf(cells[2].value, 20, "%s",  altBuf);                cells[2].label = "ALT";     cells[2].color = C_GREEN;
  snprintf(cells[3].value, 20, "%.0fkm/h", f.speed_kmh);     cells[3].label = "SPEED";   cells[3].color = TFT_WHITE;
  snprintf(cells[4].value, 20, "%.0f°",    f.heading_deg);    cells[4].label = "HDG";     cells[4].color = C_MUTED;
  snprintf(cells[5].value, 20, "%s",  iata.c_str());          cells[5].label = "AIRLINE"; cells[5].color = info.color;

  int gridY = 140;
  int cellH = 48;
  for (int i = 0; i < 6; i++) {
    int col  = i % 2;
    int row  = i / 2;
    int cx   = col == 0 ? SCREEN_W/4 : 3*SCREEN_W/4;
    int cy   = gridY + row * cellH + cellH/2;

    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setTextDatum(lgfx::middle_center);
    tft.drawString(cells[i].label, cx, cy - 10);

    tft.setTextSize(1); tft.setTextColor(cells[i].color);
    tft.drawString(cells[i].value, cx, cy + 10);

    if (col == 0) tft.drawFastVLine(SCREEN_W/2, gridY + row*cellH, cellH, C_BORDER);
    if (row < 2)  tft.drawFastHLine(0, gridY + (row+1)*cellH, SCREEN_W, C_BORDER);
  }

  // Hint
  tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Hold knob for radar", SCREEN_W/2, SCREEN_H - 12);
}

void updateAlertETA(int idx) {
  if (idx < 0 || idx >= g_flight_count) return;
  // Update ETA cell only (row 0 col 1)
  char buf[12];
  fmtETA(g_flights[idx].eta_seconds, buf, sizeof(buf));
  int cx = 3 * SCREEN_W / 4;
  int cy = 140 + 24;   // gridY + 0*cellH + cellH/2 + 10
  tft.fillRect(cx - 35, cy - 12, 70, 24, C_BG);
  tft.setTextSize(1); tft.setTextColor(C_ORANGE);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(buf, cx, cy);
}

// ── 3. LIST SCREEN ───────────────────────────────────────
// Scrollable list of up to 5 visible flights

#define LIST_ITEM_H  44
#define LIST_VISIBLE  7   // 360px tall screen fits more rows than the original 280px design

void drawListScreen(int selectedIdx) {
  tft.fillScreen(C_BG);
  drawTopBar("NEARBY FLIGHTS", g_wifi_connected);

  if (g_flight_count == 0) {
    tft.setTextColor(C_MUTED); tft.setTextDatum(lgfx::middle_center);
    tft.setTextSize(1);
    tft.drawString("No flights in range", SCREEN_W/2, SCREEN_H/2 - 10);
    tft.setTextColor(C_DIM);
    tft.drawString(g_config.location_name, SCREEN_W/2, SCREEN_H/2 + 10);
    return;
  }

  // Scroll offset: keep selectedIdx visible
  int scrollOffset = 0;
  if (selectedIdx >= LIST_VISIBLE) scrollOffset = selectedIdx - LIST_VISIBLE + 1;

  int listY = 22;

  for (int i = 0; i < LIST_VISIBLE && (i + scrollOffset) < g_flight_count; i++) {
    int fi  = i + scrollOffset;
    const Flight& f = g_flights[fi];
    bool sel = (fi == selectedIdx);

    int y = listY + i * LIST_ITEM_H;

    // Row background
    if (sel) tft.fillRect(0, y, SCREEN_W, LIST_ITEM_H, tft.color565(6, 20, 14));
    else      tft.fillRect(0, y, SCREEN_W, LIST_ITEM_H, C_BG);

    // Selection indicator
    if (sel) tft.fillRect(0, y, 3, LIST_ITEM_H, C_GREEN);

    // Airline badge
    String iata      = extractIATA(f.callsign);
    AirlineInfo info = findAirline(iata);
    drawBadge(8, y + 8, 28, 16, info.color, iata.c_str(), TFT_WHITE, 3);

    // Callsign
    tft.setTextSize(1); tft.setTextColor(sel ? TFT_WHITE : C_MUTED);
    tft.setTextDatum(lgfx::middle_left);
    tft.drawString(f.callsign, 42, y + 12);

    // Distance — color-coded
    uint16_t distColor = (f.distance_km <= g_config.alert_dist_km) ? C_RED :
                         (f.distance_km <= 4.0f) ? C_ORANGE : C_GREEN;
    char distBuf[12];
    snprintf(distBuf, sizeof(distBuf), "%.1fkm", f.distance_km);
    tft.setTextDatum(lgfx::middle_right);
    tft.setTextColor(distColor);
    tft.drawString(distBuf, SCREEN_W - 8, y + 12);

    // Altitude + speed (small row)
    char subBuf[32];
    char altBuf[12]; fmtAlt(f.altitude_m, altBuf, sizeof(altBuf));
    snprintf(subBuf, sizeof(subBuf), "%s · %.0fkm/h · %.0f°",
             altBuf, f.speed_kmh, f.heading_deg);
    tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_left);
    tft.drawString(subBuf, 42, y + 30);

    // ETA right-aligned small
    char etaBuf[12]; fmtETA(f.eta_seconds, etaBuf, sizeof(etaBuf));
    tft.setTextColor(C_ORANGE); tft.setTextDatum(lgfx::middle_right);
    tft.drawString(etaBuf, SCREEN_W - 8, y + 30);

    // Row divider
    tft.drawFastHLine(0, y + LIST_ITEM_H - 1, SCREEN_W, C_BORDER);
  }

  // Scroll indicator
  if (g_flight_count > LIST_VISIBLE) {
    tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_center);
    char buf[20];
    snprintf(buf, sizeof(buf), "%d/%d", selectedIdx + 1, g_flight_count);
    tft.drawString(buf, SCREEN_W/2, listY + LIST_VISIBLE * LIST_ITEM_H + 8);
  }
}

// ── 4. DETAIL SCREEN ─────────────────────────────────────
// Photo area (top half) + 6-cell grid (bottom half)
// aircraft_photo.cpp fills the photo area; here we draw the grid.

void drawDetailScreen(int idx) {
  if (idx < 0 || idx >= g_flight_count) return;
  const Flight& f = g_flights[idx];

  tft.fillScreen(C_BG);
  drawTopBar(f.callsign, g_wifi_connected);

  // Photo area placeholder — aircraft_photo will draw into this region
  // Region: x=0 y=20 w=360 h=180 (must match PHOTO_X/Y/W/H in aircraft_photo.cpp)
  tft.fillRect(0, 20, SCREEN_W, 180, tft.color565(8, 8, 16));
  tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(1);
  tft.drawString("Loading photo...", SCREEN_W/2, 110);

  drawDivider(200);

  // Airline badge + name strip
  String iata      = extractIATA(f.callsign);
  AirlineInfo info = findAirline(iata);
  drawBadge(8, 206, 28, 16, info.color, iata.c_str(), TFT_WHITE, 3);
  tft.setTextColor(TFT_WHITE); tft.setTextDatum(lgfx::middle_left);
  tft.drawString(info.name, 42, 214);

  // Distance badge right
  uint16_t distColor = (f.distance_km <= g_config.alert_dist_km) ? C_RED : C_GREEN;
  char distBuf[16];
  snprintf(distBuf, sizeof(distBuf), "%.1fkm", f.distance_km);
  tft.setTextColor(distColor); tft.setTextDatum(lgfx::middle_right);
  tft.drawString(distBuf, SCREEN_W - 8, 214);

  drawDivider(228);

  // 6-cell mini grid  (3 cols × 2 rows)
  struct MiniCell { const char* label; char value[16]; uint16_t color; };
  MiniCell mc[6];
  char altBuf[12]; fmtAlt(f.altitude_m, altBuf, sizeof(altBuf));
  char etaBuf[12]; fmtETA(f.eta_seconds, etaBuf, sizeof(etaBuf));

  snprintf(mc[0].value, 16, "%s",      altBuf);           mc[0].label="ALT";   mc[0].color=C_GREEN;
  snprintf(mc[1].value, 16, "%.0f°",   f.heading_deg);    mc[1].label="HDG";   mc[1].color=TFT_WHITE;
  snprintf(mc[2].value, 16, "%.0f",    f.speed_kmh);      mc[2].label="KMH";   mc[2].color=C_MUTED;
  snprintf(mc[3].value, 16, "%.1fkm",  f.distance_km);    mc[3].label="DIST";  mc[3].color=distColor;
  snprintf(mc[4].value, 16, "%s",      etaBuf);            mc[4].label="ETA";   mc[4].color=C_ORANGE;
  snprintf(mc[5].value, 16, "%s",      f.icao24);          mc[5].label="HEX";   mc[5].color=C_DIM;

  int gY   = 234;
  int colW = SCREEN_W / 3;
  int rowH = (SCREEN_H - gY - 8) / 2;

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int cx  = col * colW + colW / 2;
    int cy  = gY + row * rowH + rowH / 2;

    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setTextDatum(lgfx::middle_center);
    tft.drawString(mc[i].label, cx, cy - 10);

    tft.setTextColor(mc[i].color);
    tft.drawString(mc[i].value, cx, cy + 8);

    if (col < 2) tft.drawFastVLine((col+1)*colW, gY, rowH*2, C_BORDER);
  }
  tft.drawFastHLine(0, gY + rowH, SCREEN_W, C_BORDER);

  // Hint
  tft.setTextColor(C_DIM); tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Hold knob · Back", SCREEN_W/2, SCREEN_H - 6);
}
