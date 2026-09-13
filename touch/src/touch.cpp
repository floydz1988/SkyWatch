// ============================================================
//  touch.cpp — CST816 capacitive touch (I2C)
//  Protocol ported from Waveshare's official demo (cst816.cpp:
//  register 0x00, 7 bytes, byte[2]=touch count, x/y in bytes 3-6)
//  onto Arduino's Wire instead of raw ESP-IDF driver/i2c.h, since
//  the rest of this project is Arduino-framework throughout.
// ============================================================
#include "touch.h"
#include "config.h"
#include <Wire.h>

static bool     _touched  = false;
static uint16_t _x        = 0;
static uint16_t _y        = 0;

void touchInit() {
  Wire.begin(TOUCH_SDA, TOUCH_SCL, 300000);

  // Switch CST816 to normal mode
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
}

void touchUpdate() {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    _touched = false;
    return;
  }

  if (Wire.requestFrom((int)TOUCH_ADDR, 7) != 7) {
    _touched = false;
    return;
  }

  uint8_t data[7];
  for (int i = 0; i < 7; i++) data[i] = Wire.read();

  uint8_t count = data[2];
  if (count == 0) {
    _touched = false;
    return;
  }

  _x       = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
  _y       = ((uint16_t)(data[5] & 0x0F) << 8) | data[6];
  _touched = true;
}

bool touchTouched() { return _touched; }
uint16_t touchX()   { return _x; }
uint16_t touchY()   { return _y; }
