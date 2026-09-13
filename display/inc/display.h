#pragma once
// ============================================================
//  display.h — SH8601 QSPI AMOLED driver, 360x360
//  Waveshare ESP32-S3-Knob-Touch-LCD-1.8
//
//  Pin map and controller identity verified directly against
//  Waveshare's official demo firmware (08_LVGL_Test/lcd_config.h,
//  esp_lcd_sh8601.c) — the original LovyanGFX/ST7789/240x280 setup
//  was never correct for this hardware; LovyanGFX also has no
//  SH8601/QSPI driver at all.
//
//  This facade keeps the same tft.* call-site API the rest of the
//  codebase already uses (screens.cpp, screensaver.cpp,
//  aircraft_photo.cpp, SkyWatch.ino) so only this file and
//  display.cpp needed to change graphics libraries — not every
//  draw call site.
// ============================================================
#include <Arduino_GFX_Library.h>
#include "config.h"

// ── Text datum constants (mirrors the subset of LovyanGFX's
//    lgfx::datum_t this project actually uses) ─────────────────
namespace lgfx {
  enum datum_t {
    top_left,
    middle_left,
    middle_center,
    middle_right,
    bottom_center
  };
}

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000

// ── LGFX facade — Arduino_GFX (Arduino_ESP32QSPI + Arduino_SH8601)
//    underneath, exposing the same method surface the drawing code
//    already calls. ─────────────────────────────────────────────
class LGFX {
public:
  LGFX();

  void init();
  void setRotation(uint8_t r);
  void setBrightness(uint8_t level);   // 0-255, PWM on LCD_BL

  void fillScreen(uint16_t color);
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color);
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color);
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  void setTextSize(uint8_t size);
  void setTextColor(uint16_t color);
  void setTextDatum(lgfx::datum_t datum);
  void drawString(const char* text, int32_t x, int32_t y);

  // Matches the old LovyanGFX drawJpg(data,len,x,y,maxW,maxH,offX,offY,datum)
  // signature used throughout — decodes via TJpg_Decoder, blits via
  // Arduino_GFX's draw16bitRGBBitmap, centered/positioned per datum
  // within the maxW x maxH box the same way the old call sites expect.
  bool drawJpg(const uint8_t* data, uint32_t len,
               int32_t x, int32_t y, int32_t maxW, int32_t maxH,
               int32_t offX, int32_t offY, lgfx::datum_t datum = lgfx::top_left);

  Arduino_GFX* raw() { return _gfx; }

private:
  Arduino_DataBus*  _bus;
  Arduino_SH8601*   _gfx;
  lgfx::datum_t     _datum = lgfx::top_left;
};

// Global display instance — include display.h everywhere you need to draw
extern LGFX tft;

// ── Helper functions ─────────────────────────────────────────
void displayInit();
void displayBrightness(uint8_t level);   // 0–255
void displayClear(uint16_t color = C_BG);

// Draw a rounded-rectangle badge (airline badge, tags, etc.)
void drawBadge(int x, int y, int w, int h,
               uint16_t bg, const char* text, uint16_t fg = C_WHITE, int radius = 5);

// Draw top status bar common to all screens
void drawTopBar(const char* title, bool wifiOk);
