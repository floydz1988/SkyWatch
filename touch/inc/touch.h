#pragma once
// ============================================================
//  touch.h — CST816 capacitive touch (I2C)
//  This knob has no physical push button — the touchscreen is the
//  "press" input, read by knob.cpp to drive its existing
//  press/long-press state machine.
// ============================================================
#include <Arduino.h>

void touchInit();
void touchUpdate();         // call once per poll (knobUpdate() does this)
bool touchTouched();        // true while a finger is on the screen

// Last known touch coordinates (valid only while touchTouched() is true)
uint16_t touchX();
uint16_t touchY();
