#pragma once
// ============================================================
//  airlines.h — Airline brand colour + name lookup
//  Add more entries as needed. Covers major European carriers.
// ============================================================
#include <Arduino.h>
#include <pgmspace.h>

struct AirlineInfo {
  const char* iata;     // 2-letter IATA code
  uint16_t    color;    // RGB565 brand colour
  const char* name;     // full name
};

// RGB565 macro:  R(5bit) G(6bit) B(5bit)
// (Arduino_GFX also defines this identically — guard against the redefinition
// warning when both headers land in the same translation unit, e.g. screens.cpp.)
#ifndef RGB565
#define RGB565(r,g,b) ((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3))
#endif

// Airline DB (defined in airlines.cpp), stored in PROGMEM
extern const AirlineInfo AIRLINE_DB[];
extern const int         AIRLINE_DB_SIZE;

// Fallback for unknown airlines
extern const AirlineInfo AIRLINE_UNKNOWN;

// Extract IATA from callsign: "DLH421" -> "LH", "RYR552" -> "FR"
// Uses the ICAO airline prefix (3-letter) -> IATA mapping for common ones.
// Simpler approach: strip trailing digits, look up first 2 chars.
String extractIATA(const char* callsign);

// Find airline by IATA code (returns AIRLINE_UNKNOWN if not found)
AirlineInfo findAirline(const String& iata);
