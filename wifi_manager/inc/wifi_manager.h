#pragma once
// ============================================================
//  wifi_manager.h — WiFi connection + captive config portal
//  Hold knob 5 sec on screensaver → AP "SkyWatch-Setup" starts
//  Connect from phone/laptop → upload logo + set location
// ============================================================
#include <Arduino.h>

// Saved config (loaded from LittleFS/config.json at boot)
struct AppConfig {
  char  ssid[64];
  char  password[64];
  char  owner_name[32];
  char  location_name[16];  // e.g. "HAM"
  float lat;
  float lon;
  float radius_km;
  float alert_dist_km;
};

extern AppConfig g_config;
extern bool      g_wifi_connected;

bool  wifiLoadConfig();         // read config.json from LittleFS
bool  wifiConnect();            // connect using saved credentials
void  wifiStartSetupPortal();   // AP + web server for first-time setup
void  wifiHandlePortal();       // call in loop() while portal is active
bool  wifiPortalActive();
