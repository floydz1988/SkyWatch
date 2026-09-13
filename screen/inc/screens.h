#pragma once
// ============================================================
//  screens.h — Full-screen draw routines for each UI state
// ============================================================
#include <Arduino.h>

// Called once per state entry (full redraw)
void drawRadarScreen();
void drawAlertScreen(int flightIdx);
void drawListScreen(int selectedIdx);
void drawDetailScreen(int flightIdx);

// Called every loop() for animated elements (radar sweep etc.)
// Returns true if a redraw was performed.
bool tickRadar();

// Update ETA countdown on alert screen without full redraw
void updateAlertETA(int flightIdx);
