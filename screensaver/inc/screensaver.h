#pragma once
// ============================================================
//  screensaver.h — Custom logo screensaver
//  Reads /logo.jpg from LittleFS (uploaded via web portal).
//  Falls back to built-in "SkyWatch" branding if no logo.
//  Exits when knob is pressed/rotated.
// ============================================================
#include <Arduino.h>

// Draw the screensaver (once).  Call when entering STATE_SCREENSAVER.
void screensaverDraw();

// Animate / refresh screensaver elements (call every loop()).
// Currently: subtle pulse on branding text.  No-op if logo is shown.
void screensaverTick();
