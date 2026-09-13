// ============================================================
//  flight.cpp — Shared flight list storage
// ============================================================
#include "flight.h"

Flight g_flights[MAX_FLIGHTS];
int    g_flight_count = 0;
int    g_selected_idx = 0;
int    g_closest_idx  = -1;
