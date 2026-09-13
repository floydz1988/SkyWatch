// ============================================================
//  display.cpp — SH8601 QSPI AMOLED driver implementation
//  (Arduino_GFX + TJpg_Decoder underneath — see display.h)
// ============================================================
#include "display.h"
#include <TJpg_Decoder.h>

LGFX tft;

// ── JPEG decode → display blit ──────────────────────────────
// TJpg_Decoder's callback is a plain function pointer, not a member,
// so it reaches the active display through this file-scope pointer.
// Only one LGFX instance ever exists in this project.
static Arduino_GFX* _jpgTarget = nullptr;

static bool jpgOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* data) {
  if (!_jpgTarget) return false;
  _jpgTarget->draw16bitRGBBitmap(x, y, data, w, h);
  return true;
}

// ── LGFX facade ──────────────────────────────────────────────
LGFX::LGFX() {
  _bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
  _gfx = new Arduino_SH8601(_bus, LCD_RST, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 0);
}

void LGFX::init() {
  _gfx->begin();

  // Backlight PWM — 50kHz / 8-bit, matching Waveshare's own reference
  // (lcd_bl_pwm_bsp.c) and this project's existing BL_FULL=255/BL_DIM=25 scale.
  ledcAttach(LCD_BL, 50000, 8);
  ledcWrite(LCD_BL, BL_FULL);

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(jpgOutputCallback);
}

void LGFX::setRotation(uint8_t r)        { _gfx->setRotation(r); }
void LGFX::setBrightness(uint8_t level)  { ledcWrite(LCD_BL, level); }

void LGFX::fillScreen(uint16_t color)                                    { _gfx->fillScreen(color); }
void LGFX::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
                                                                           { _gfx->fillRect(x, y, w, h, color); }
void LGFX::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
                                                                           { _gfx->drawRect(x, y, w, h, color); }
void LGFX::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color)
                                                                           { _gfx->fillRoundRect(x, y, w, h, radius, color); }
void LGFX::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) { _gfx->drawFastHLine(x, y, w, color); }
void LGFX::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) { _gfx->drawFastVLine(x, y, h, color); }
void LGFX::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
                                                                           { _gfx->drawLine(x0, y0, x1, y1, color); }
void LGFX::drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color)    { _gfx->drawCircle(x, y, r, color); }
void LGFX::fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color)    { _gfx->fillCircle(x, y, r, color); }
void LGFX::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color)
                                                                           { _gfx->fillTriangle(x0, y0, x1, y1, x2, y2, color); }

uint16_t LGFX::color565(uint8_t r, uint8_t g, uint8_t b) { return _gfx->color565(r, g, b); }

void LGFX::setTextSize(uint8_t size)          { _gfx->setTextSize(size); }
void LGFX::setTextColor(uint16_t color)       { _gfx->setTextColor(color); }
void LGFX::setTextDatum(lgfx::datum_t datum)  { _datum = datum; }

void LGFX::drawString(const char* text, int32_t x, int32_t y) {
  int16_t x1, y1;
  uint16_t w, h;
  _gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int32_t cx, cy;
  switch (_datum) {
    case lgfx::middle_center: cx = x - (int32_t)w / 2 - x1; cy = y - (int32_t)h / 2 - y1; break;
    case lgfx::middle_left:   cx = x - x1;                   cy = y - (int32_t)h / 2 - y1; break;
    case lgfx::middle_right:  cx = x - (int32_t)w - x1;       cy = y - (int32_t)h / 2 - y1; break;
    case lgfx::bottom_center: cx = x - (int32_t)w / 2 - x1;   cy = y - (int32_t)h - y1;     break;
    default:                  cx = x - x1;                    cy = y - y1;                   break;
  }
  _gfx->setCursor(cx, cy);
  _gfx->print(text);
}

bool LGFX::drawJpg(const uint8_t* data, uint32_t len,
                    int32_t x, int32_t y, int32_t maxW, int32_t maxH,
                    int32_t offX, int32_t offY, lgfx::datum_t datum) {
  uint16_t jw, jh;
  if (TJpgDec.getJpgSize(&jw, &jh, data, len) != JDR_OK) return false;

  int32_t drawX = x + offX;
  int32_t drawY = y + offY;
  if (datum == lgfx::middle_center) {
    drawX = x + (maxW - (int32_t)jw) / 2;
    drawY = y + (maxH - (int32_t)jh) / 2;
  }

  _jpgTarget = _gfx;
  return TJpgDec.drawJpg(drawX, drawY, data, len) == JDR_OK;
}

// ── Public init/helpers ──────────────────────────────────────
void displayInit() {
  tft.init();
  tft.setRotation(0);
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
