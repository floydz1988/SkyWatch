#pragma once
// ============================================================
//  display.h — LovyanGFX driver for ST7789 240×280
//  Waveshare ESP32-S3-Knob-Touch-LCD-1.8
// ============================================================
#include <LovyanGFX.hpp>
#include "config.h"

// ── LGFX hardware config ─────────────────────────────────────
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;

public:
  LGFX() {
    // SPI bus
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 80000000;
      cfg.freq_read   = 20000000;
      cfg.spi_3wire   = true;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = LCD_SCLK;
      cfg.pin_mosi    = LCD_MOSI;
      cfg.pin_miso    = -1;
      cfg.pin_dc      = LCD_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    // Panel
    {
      auto cfg = _panel.config();
      cfg.pin_cs          = LCD_CS;
      cfg.pin_rst         = LCD_RST;
      cfg.pin_busy        = -1;
      cfg.memory_width    = 240;
      cfg.memory_height   = 280;
      cfg.panel_width     = 240;
      cfg.panel_height    = 280;
      cfg.offset_x        = 0;
      cfg.offset_y        = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable        = false;
      cfg.invert          = true;   // ST7789 needs inversion
      cfg.rgb_order       = false;
      cfg.dlen_16bit      = false;
      cfg.bus_shared      = false;
      _panel.config(cfg);
    }
    // Backlight (PWM)
    {
      auto cfg = _light.config();
      cfg.pin_bl      = LCD_BL;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
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
