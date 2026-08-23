#include "RemoteDisplayWebServer.h"
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

namespace {
constexpr const char* FALLBACK_PAGE =
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Remote Display Wi-Fi</title></head><body><h1>Remote Display Wi-Fi</h1>"
    "<form method='post' action='/savewifi'><label>SSID <input name='ssid' required></label><br>"
    "<label>Password <input name='password' type='password'></label><br><button>Save and restart</button></form></body></html>";
constexpr const char* FALLBACK_STYLE = "body{font-family:Arial;margin:2rem;max-width:30rem}input,button{margin:.5rem;padding:.6rem;width:100%;box-sizing:border-box}";
}

String RemoteDisplayWebServer::readFileOrFallback(const char* path, const char* fallback) const {
    File file = SPIFFS.open(path, "r");
    if (!file) return String(fallback);
    String content = file.readString();
    file.close();
    return content.isEmpty() ? String(fallback) : content;
}

void RemoteDisplayWebServer::begin() {
    _server.on("/", HTTP_GET, [this] { handleRoot(); });
    _server.on("/wifi", HTTP_GET, [this] { handleRoot(); });
    _server.on("/style.css", HTTP_GET, [this] { handleStyle(); });
    _server.on("/savewifi", HTTP_POST, [this] { handleWifiSave(); });
    _server.onNotFound([this] { _server.sendHeader("Location", "/", true); _server.send(302, "text/plain", "Redirecting"); });
    _server.begin();
    Serial.println("Remote display web server started");
}

void RemoteDisplayWebServer::handleRoot() { _server.send(200, "text/html", readFileOrFallback("/wifi.html", FALLBACK_PAGE)); }
void RemoteDisplayWebServer::handleStyle() { _server.send(200, "text/css", readFileOrFallback("/style.css", FALLBACK_STYLE)); }

void RemoteDisplayWebServer::handleWifiSave() {
    const String ssid = _server.arg("ssid");
    const String password = _server.arg("password");
    if (ssid.isEmpty()) { _server.send(400, "text/plain", "SSID is required"); return; }
    Preferences preferences;
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    _server.send(200, "text/html", "<html><body>Wi-Fi credentials saved. Restarting...</body></html>");
    _restartAtMs = millis() + 500;
}

void RemoteDisplayWebServer::update() {
    _server.handleClient();
    if (_restartAtMs != 0 && static_cast<int32_t>(millis() - _restartAtMs) >= 0) ESP.restart();
}
