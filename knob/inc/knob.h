#pragma once
// ============================================================
//  knob.h — Rotary encoder + push button input
// ============================================================
#include <Arduino.h>
#include "config.h"

void    knobInit();
void    knobUpdate();          // call every loop()

int8_t  knobDelta();           // +1 CW, -1 CCW since last call
bool    knobPressed();         // true once on short press
bool    knobLongPressed();     // true once on long press
