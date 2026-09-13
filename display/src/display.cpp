// ============================================================
//  display.cpp — LovyanGFX display implementation
// ============================================================
#include "display.h"

LGFX tft;

void displayInit() {
  tft.init();
  tft.setRotation(0);          // portrait, USB at bottom
  tft.fillScreen(C_BG);
  tft.setBrightness(BL_FULL);
}

void displayBrightness(uint8_t level) {
  tft.setBrightness(level);
}

void displayClear(uint16_t color) {
  tft.fillScreen(color);
}

void drawBadge(int x, int y, int w, int h,
               uint16_t bg, const char* text, uint16_t fg, int radius) {
  tft.fillRoundRect(x, y, w, h, radius, bg);
  tft.setTextColor(fg);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void drawTopBar(const char* title, bool wifiOk) {
  // Background strip
  tft.fillRect(0, 0, SCREEN_W, 20, tft.color565(4, 4, 12));
  tft.drawFastHLine(0, 20, SCREEN_W, C_BORDER);

  // Title
  tft.setTextColor(C_MUTED);
  tft.setTextDatum(lgfx::middle_left);
  tft.setTextSize(1);
  tft.drawString(title, 8, 10);

  // WiFi indicator dot
  uint16_t dotColor = wifiOk ? C_GREEN : C_RED;
  tft.fillCircle(SCREEN_W - 10, 10, 3, dotColor);
}
