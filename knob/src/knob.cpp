// ============================================================
//  knob.cpp — Rotary encoder + debounced button
// ============================================================
#include "knob.h"

static volatile int8_t _delta     = 0;
static int8_t  _lastA              = HIGH;
static int8_t  _lastB              = HIGH;

// Button debounce
static bool    _btnPressed         = false;
static bool    _btnLongPressed     = false;
static bool    _lastBtnState       = HIGH;
static uint32_t _btnDownAt         = 0;
static bool    _longFired          = false;

void knobInit() {
  pinMode(ENC_A,  INPUT_PULLUP);
  pinMode(ENC_B,  INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
}

void knobUpdate() {
  // ── Encoder rotation ──────────────────────────────────────
  int8_t a = digitalRead(ENC_A);
  int8_t b = digitalRead(ENC_B);
  if (a != _lastA) {
    _delta += (a != b) ? +1 : -1;
    _lastA = a;
  }
  _lastB = b;

  // ── Button ────────────────────────────────────────────────
  bool btnNow = (digitalRead(ENC_SW) == LOW);   // active LOW

  if (btnNow && !_lastBtnState) {
    // Press start
    _btnDownAt  = millis();
    _longFired  = false;
  }

  if (btnNow && !_longFired) {
    if (millis() - _btnDownAt >= LONG_PRESS_MS) {
      _btnLongPressed = true;
      _longFired      = true;
    }
  }

  if (!btnNow && _lastBtnState) {
    // Release
    if (!_longFired) {
      _btnPressed = true;   // short press on release
    }
  }

  _lastBtnState = btnNow;
}

int8_t knobDelta() {
  int8_t d = _delta;
  _delta = 0;
  return d;
}

bool knobPressed() {
  if (_btnPressed) { _btnPressed = false; return true; }
  return false;
}

bool knobLongPressed() {
  if (_btnLongPressed) { _btnLongPressed = false; return true; }
  return false;
}
