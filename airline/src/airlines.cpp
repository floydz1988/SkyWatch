// ============================================================
//  airlines.cpp — Airline brand colour + name lookup
// ============================================================
#include "airlines.h"

const AirlineInfo AIRLINE_DB[] PROGMEM = {
  // European carriers
  { "LH", RGB565(  0, 63,143), "Lufthansa"        },
  { "FR", RGB565(  7, 53,144), "Ryanair"           },
  { "EW", RGB565(212,  0,  0), "Eurowings"         },
  { "AB", RGB565(255,102,  0), "Air Berlin"        },
  { "EK", RGB565(192,146, 20), "Emirates"          },
  { "BA", RGB565(  0, 53,128), "British Airways"   },
  { "TU", RGB565(204,  0,  0), "TUI fly"           },
  { "DE", RGB565(227,  6, 19), "Condor"            },
  { "AF", RGB565(  0, 35,149), "Air France"        },
  { "KL", RGB565(  0,163,224), "KLM"               },
  { "SK", RGB565(  0, 40,130), "SAS"               },
  { "AY", RGB565(  0, 89,166), "Finnair"           },
  { "IB", RGB565(255,  0,  0), "Iberia"            },
  { "VY", RGB565(  0,170, 50), "Vueling"           },
  { "W6", RGB565(130,  0,  0), "Wizz Air"          },
  { "U2", RGB565(255,103,  0), "easyJet"           },
  { "TK", RGB565(220, 50, 50), "Turkish Airlines"  },
  { "OS", RGB565(237, 28, 46), "Austrian"          },
  { "LX", RGB565(134,  0, 56), "Swiss"             },
  { "SN", RGB565(  0,  0,160), "Brussels Airlines" },
  // Global
  { "QR", RGB565(122,  0, 25), "Qatar Airways"     },
  { "EY", RGB565( 85, 52, 30), "Etihad"            },
  { "SQ", RGB565( 30, 40,100), "Singapore Air"     },
  { "CX", RGB565(  0, 88,136), "Cathay Pacific"    },
  { "NH", RGB565(  0,  0,160), "ANA"               },
  { "DL", RGB565(  0, 58,112), "Delta"             },
  { "AA", RGB565(  0,  0,160), "American Airlines" },
  { "UA", RGB565(  0, 46,143), "United"            },
};

const int AIRLINE_DB_SIZE = sizeof(AIRLINE_DB) / sizeof(AIRLINE_DB[0]);

const AirlineInfo AIRLINE_UNKNOWN = { "??", RGB565(42,42,58), "Unknown" };

String extractIATA(const char* callsign) {
  // Common ICAO -> IATA prefix map
  struct { const char* icao; const char* iata; } map[] = {
    {"DLH","LH"},{"RYR","FR"},{"EWG","EW"},{"UAE","EK"},{"BAW","BA"},
    {"TUI","TU"},{"CFG","DE"},{"AFR","AF"},{"KLM","KL"},{"SAS","SK"},
    {"FIN","AY"},{"IBE","IB"},{"VLG","VY"},{"WZZ","W6"},{"EZY","U2"},
    {"THY","TK"},{"AUA","OS"},{"SWR","LX"},{"BEL","SN"},{"QTR","QR"},
    {"ETD","EY"},{"SIA","SQ"},{"CPA","CX"},{"ANA","NH"},{"DAL","DL"},
    {"AAL","AA"},{"UAL","UA"},
  };
  char prefix[4] = {0};
  strncpy(prefix, callsign, 3);
  for (auto& m : map) {
    if (strcmp(prefix, m.icao) == 0) return String(m.iata);
  }
  // fallback: first 2 chars
  char iata[3] = {callsign[0], callsign[1], 0};
  return String(iata);
}

AirlineInfo findAirline(const String& iata) {
  for (int i = 0; i < AIRLINE_DB_SIZE; i++) {
    AirlineInfo a;
    memcpy_P(&a, &AIRLINE_DB[i], sizeof(AirlineInfo));
    if (iata.equals(a.iata)) return a;
  }
  return AIRLINE_UNKNOWN;
}
