#pragma once
// ============================================================
//  config.h — SkyWatch · Waveshare ESP32-S3-Knob-Touch-LCD-1.8
//  All pins, colours, and tunable constants in one place.
//
//  Pin map verified directly against Waveshare's official demo firmware
//  (ESP32-S3-Knob-Touch-LCD-1.8-Demo.zip, 08_LVGL_Test/lcd_config.h and
//  04_Encoder_Test/04_Encoder_Test.ino) — not assumed.
// ============================================================

// ── PIN MAP ─────────────────────────────────────────────────
// SH8601 QSPI AMOLED display, 360x360
#define LCD_CS       14
#define LCD_SCLK     13   // QSPI clock (PCLK)
#define LCD_SDIO0    15   // QSPI data 0
#define LCD_SDIO1    16   // QSPI data 1
#define LCD_SDIO2    17   // QSPI data 2
#define LCD_SDIO3    18   // QSPI data 3
#define LCD_RST      21
#define LCD_BL       47   // backlight PWM

// Rotary encoder (rotation only — this knob has no physical push button;
// "select" is a tap on the touchscreen instead, see knob.cpp)
#define ENC_A         8
#define ENC_B         7

// Capacitive touch (CST816, I2C) — doubles as the knob's "press" input
#define TOUCH_SDA    11
#define TOUCH_SCL    12
#define TOUCH_ADDR   0x15

// ── DISPLAY ─────────────────────────────────────────────────
#define SCREEN_W    360
#define SCREEN_H    360
#define BL_FULL     255
#define BL_DIM       25   // ~10% — night mode
#define DIM_AFTER_MS (10UL * 60 * 1000)  // 10 minutes

// ── COLOURS (RGB565) ─────────────────────────────────────────
#define C_BLACK     0x0000
#define C_WHITE     0xFFFF
#define C_BG        0x0103   // near-black #030811
#define C_SURFACE   0x0866   // dark surface
#define C_BORDER    0x1082   // subtle border
#define C_GREEN     0x072C   // #00E5A0 radar green
#define C_ORANGE    0xFA43   // #FF4D1C aviation orange
#define C_AMBER     0xFD00   // #FF9F0A
#define C_RED       0xF8C6   // #FF3B30
#define C_MUTED     0x39CE   // #7A7A8A
#define C_DIM       0x14A3   // #292935

// ── FEATURE FLAGS ─────────────────────────────────────────────
// Set to 0 to compile a feature out entirely (reclaims its flash/RAM
// footprint for other features). Set back to 1 to re-enable — no other
// code changes needed, call sites stay in place as no-ops when disabled.
#define FEATURE_RECORDS   0   // Personal records (closest/fastest/highest/total-seen), NVS-backed

// ── APP BEHAVIOUR ────────────────────────────────────────────
#define OPENSKY_INTERVAL_MS   10000   // fetch every 10 sec
#define ALERT_DIST_KM           2.0f  // trigger alert screen
#define SCAN_RADIUS_KM         10.0f  // default search radius
#define MAX_FLIGHTS             20    // max planes to track
#define LONG_PRESS_MS          800    // long press threshold
#define PHOTO_TIMEOUT_MS      5000    // give up fetching photo

// ── FIRMWARE ────────────────────────────────────────────────
#define FW_VERSION   "1.0.0"
#define DEVICE_NAME  "SkyWatch"

// ── SCREEN STATES ────────────────────────────────────────────
enum ScreenState {
  STATE_SCREENSAVER,
  STATE_RADAR,
  STATE_ALERT,
  STATE_LIST,
  STATE_DETAIL
};
