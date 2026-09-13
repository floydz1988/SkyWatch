// ============================================================
//  wifi_manager.cpp — WiFi + captive config portal
// ============================================================
#include "wifi_manager.h"
#include "display.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

AppConfig g_config;
bool      g_wifi_connected = false;

static WebServer _server(80);
static bool      _portalActive = false;

// ── Default config fallback ──────────────────────────────────
static void setDefaults() {
  strncpy(g_config.ssid,          "",           sizeof(g_config.ssid));
  strncpy(g_config.password,      "",           sizeof(g_config.password));
  strncpy(g_config.owner_name,    "SkyWatch",   sizeof(g_config.owner_name));
  strncpy(g_config.location_name, "HOME",       sizeof(g_config.location_name));
  g_config.lat          = 53.7050f;   // Norderstedt, Germany
  g_config.lon          = 10.0110f;
  g_config.radius_km    = 10.0f;
  g_config.alert_dist_km = 2.0f;
}

// ── Load config.json from LittleFS ──────────────────────────
bool wifiLoadConfig() {
  setDefaults();
  if (!LittleFS.begin(false)) return false;
  if (!LittleFS.exists("/config.json")) return false;

  File f = LittleFS.open("/config.json", "r");
  if (!f) return false;

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return false; }
  f.close();

  strlcpy(g_config.ssid,          doc["ssid"]          | "", sizeof(g_config.ssid));
  strlcpy(g_config.password,      doc["password"]      | "", sizeof(g_config.password));
  strlcpy(g_config.owner_name,    doc["owner_name"]    | "SkyWatch", sizeof(g_config.owner_name));
  strlcpy(g_config.location_name, doc["location_name"] | "HOME",     sizeof(g_config.location_name));
  g_config.lat           = doc["lat"]           | 53.7050f;
  g_config.lon           = doc["lon"]           | 10.0110f;
  g_config.radius_km     = doc["radius_km"]     | 10.0f;
  g_config.alert_dist_km = doc["alert_dist_km"] | 2.0f;
  return true;
}

// ── Save config.json to LittleFS ─────────────────────────────
static void saveConfig(const AppConfig& cfg) {
  StaticJsonDocument<512> doc;
  doc["ssid"]           = cfg.ssid;
  doc["password"]       = cfg.password;
  doc["owner_name"]     = cfg.owner_name;
  doc["location_name"]  = cfg.location_name;
  doc["lat"]            = cfg.lat;
  doc["lon"]            = cfg.lon;
  doc["radius_km"]      = cfg.radius_km;
  doc["alert_dist_km"]  = cfg.alert_dist_km;

  File f = LittleFS.open("/config.json", "w");
  if (f) { serializeJson(doc, f); f.close(); }
}

// ── Connect to WiFi ──────────────────────────────────────────
bool wifiConnect() {
  if (strlen(g_config.ssid) == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(g_config.ssid, g_config.password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(200);
  }
  g_wifi_connected = (WiFi.status() == WL_CONNECTED);
  return g_wifi_connected;
}

// ── Setup portal HTML page ───────────────────────────────────
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SkyWatch Setup</title>
<style>
  body{font-family:system-ui,sans-serif;background:#0E0E12;color:#EDEDF0;
       max-width:420px;margin:0 auto;padding:24px 16px}
  h1{color:#00E5A0;font-size:1.4rem;margin-bottom:4px}
  .sub{color:#5A5A7A;font-size:.85rem;margin-bottom:24px}
  label{display:block;font-size:.8rem;color:#7A7A8A;margin:12px 0 4px}
  input{width:100%;padding:10px;background:#17171D;border:1px solid #2A2A38;
        border-radius:6px;color:#EDEDF0;font-size:.9rem;box-sizing:border-box}
  input:focus{outline:none;border-color:#00E5A0}
  .row{display:flex;gap:8px}
  .row input{flex:1}
  button{width:100%;margin-top:20px;padding:12px;background:#FF4D1C;
         border:none;border-radius:8px;color:#fff;font-size:1rem;
         font-weight:600;cursor:pointer}
  .msg{margin-top:16px;padding:10px;border-radius:6px;font-size:.85rem;display:none}
  .ok{background:rgba(0,229,160,.15);color:#00E5A0;border:1px solid rgba(0,229,160,.3)}
  .upload{margin-top:20px;padding:16px;background:#17171D;
          border-radius:8px;border:1px solid #2A2A38}
  .upload h3{font-size:.9rem;margin:0 0 8px;color:#EDEDF0}
</style></head><body>
<h1>✈ SkyWatch Setup</h1>
<div class="sub">Configure your device once — saved to flash.</div>
<form id="frm">
  <label>WiFi Network Name (SSID)</label>
  <input name="ssid" placeholder="Your WiFi name" required>
  <label>WiFi Password</label>
  <input name="password" type="password" placeholder="Your WiFi password">
  <label>Your Name / Owner</label>
  <input name="owner_name" placeholder="e.g. Raj" maxlength="31">
  <label>Location Code (shown on screen)</label>
  <input name="location_name" placeholder="e.g. HAM" maxlength="8">
  <label>Home Coordinates</label>
  <div class="row">
    <input name="lat" type="number" step="0.0001" placeholder="Latitude" required>
    <input name="lon" type="number" step="0.0001" placeholder="Longitude" required>
  </div>
  <label>Scan radius (km)</label>
  <input name="radius_km" type="number" step="0.5" value="10" min="1" max="50">
  <label>Alert distance (km)</label>
  <input name="alert_dist_km" type="number" step="0.5" value="2" min="0.5" max="10">
  <button type="submit">💾 Save & Connect</button>
</form>
<div class="upload">
  <h3>🖼 Custom Screensaver Logo</h3>
  <input type="file" id="logoFile" accept="image/jpeg,image/png">
  <button onclick="uploadLogo()">Upload Logo</button>
</div>
<div class="msg ok" id="msg"></div>
<script>
document.getElementById('frm').onsubmit=async e=>{
  e.preventDefault();
  const d=Object.fromEntries(new FormData(e.target));
  const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)});
  const m=document.getElementById('msg');
  m.style.display='block';
  if(r.ok){m.textContent='✅ Saved! Device will restart and connect to WiFi.';m.className='msg ok';}
  else{m.textContent='❌ Error saving.';m.className='msg';}
};
async function uploadLogo(){
  const f=document.getElementById('logoFile').files[0];
  if(!f)return;
  const fd=new FormData();fd.append('logo',f);
  await fetch('/upload',{method:'POST',body:fd});
  const m=document.getElementById('msg');
  m.style.display='block';m.textContent='✅ Logo uploaded!';m.className='msg ok';
}
</script></body></html>
)rawliteral";

// ── Portal route handlers ─────────────────────────────────────
static void handleRoot() {
  _server.send_P(200, "text/html", PORTAL_HTML);
}

static void handleSave() {
  if (!_server.hasArg("plain")) { _server.send(400); return; }
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, _server.arg("plain")) != DeserializationError::Ok) {
    _server.send(400); return;
  }
  strlcpy(g_config.ssid,          doc["ssid"]          | "", sizeof(g_config.ssid));
  strlcpy(g_config.password,      doc["password"]      | "", sizeof(g_config.password));
  strlcpy(g_config.owner_name,    doc["owner_name"]    | "", sizeof(g_config.owner_name));
  strlcpy(g_config.location_name, doc["location_name"] | "", sizeof(g_config.location_name));
  g_config.lat           = doc["lat"]           | 53.705f;
  g_config.lon           = doc["lon"]           | 10.011f;
  g_config.radius_km     = doc["radius_km"]     | 10.0f;
  g_config.alert_dist_km = doc["alert_dist_km"] | 2.0f;
  saveConfig(g_config);
  _server.send(200, "application/json", "{\"ok\":true}");
  delay(1500);
  ESP.restart();
}

static void handleUpload() {
  HTTPUpload& upload = _server.upload();
  static File uploadFile;
  if (upload.status == UPLOAD_FILE_START) {
    uploadFile = LittleFS.open("/logo.jpg", "w");
  } else if (upload.status == UPLOAD_FILE_WRITE && uploadFile) {
    uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END && uploadFile) {
    uploadFile.close();
    _server.send(200, "application/json", "{\"ok\":true}");
  }
}

// ── Start AP + web server ─────────────────────────────────────
void wifiStartSetupPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SkyWatch-Setup", "skywatch1");

  _server.on("/",         HTTP_GET,  handleRoot);
  _server.on("/save",     HTTP_POST, handleSave);
  _server.on("/upload",   HTTP_POST, [](){_server.send(200);}, handleUpload);
  _server.onNotFound([](){ _server.sendHeader("Location","/"); _server.send(302); });
  _server.begin();
  _portalActive = true;
}

void wifiHandlePortal() {
  if (_portalActive) _server.handleClient();
}

bool wifiPortalActive() { return _portalActive; }
