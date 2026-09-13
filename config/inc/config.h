#pragma once
// ============================================================
//  config.h — SkyWatch · Waveshare ESP32-S3-Knob-Touch-LCD-1.8
//  All pins, colours, and tunable constants in one place.
//  Verify pin numbers against your board's Wiki before flashing.
// ============================================================

// ── PIN MAP ─────────────────────────────────────────────────
// ST7789 SPI display
#define LCD_MOSI     11
#define LCD_SCLK     12
#define LCD_CS       10
#define LCD_DC        8
#define LCD_RST       9
#define LCD_BL       46   // backlight PWM

// Rotary encoder
#define ENC_A         1
#define ENC_B         2
#define ENC_SW        0   // push button (active LOW)

// Capacitive touch (CST816S I2C) — optional, not used in v1
#define TOUCH_SDA     4
#define TOUCH_SCL     5
#define TOUCH_INT     3

// ── DISPLAY ─────────────────────────────────────────────────
#define SCREEN_W    240
#define SCREEN_H    280
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
