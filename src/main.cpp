#include <Arduino.h>

#include <WiFi.h>
#include <Preferences.h>
#include <SPIFFS.h>

#include "SimpleLEDMatrix.h"

#include "RemoteDisplayConfig.h"

#include "RemoteDisplayReceiver.h"
#include "RemoteDisplayWebServer.h"

namespace {
    constexpr uint8_t DISPLAY_PIN = 21;
    SimpleLEDMatrix matrix(DISPLAY_PIN, 64, 16, 250);
    uint32_t lastMatrixRefreshMs = 0;
    uint32_t lastDisplayedTimeMs = UINT32_MAX;
    void displayTime(uint32_t ms, bool force) {
        const uint32_t now = millis();
        if (!force && (ms == lastDisplayedTimeMs || now - lastMatrixRefreshMs < REMOTE_DISPLAY_MATRIX_REFRESH_MS)) return;
        matrix.showTime(ms, 0, 255, 0);
        lastMatrixRefreshMs = now;
        lastDisplayedTimeMs = ms;
    }
    void displayIdle() {
        matrix.showTime(0, 0, 255, 0);
        lastDisplayedTimeMs = 0;
    }
    void displayOffline() {
        matrix.showText("OFFLINE", 255, 80, 0);
    }
    RemoteDisplayReceiver receiver(displayTime, displayIdle, displayOffline);
    RemoteDisplayWebServer webServer;
    bool hasWifiCredentials = false;
    bool provisioningAccessPoint = false;
    uint32_t wifiConnectStartedMs = 0;

    void startProvisioningAccessPoint() {
        if (provisioningAccessPoint) return;
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(REMOTE_DISPLAY_AP_SSID, REMOTE_DISPLAY_AP_PASSWORD);
        provisioningAccessPoint = true;
        Serial.printf("Wi-Fi setup AP: %s, IP: %s\n", REMOTE_DISPLAY_AP_SSID, WiFi.softAPIP().toString().c_str());
    }

    void maintainWifi() {
        if (!hasWifiCredentials || provisioningAccessPoint || WiFi.status() == WL_CONNECTED) return;
        if (millis() - wifiConnectStartedMs >= REMOTE_DISPLAY_WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("Wi-Fi connection timed out; starting setup AP");
            startProvisioningAccessPoint();
        }
    }
}
void setup() {
    Serial.begin(115200);
    matrix.begin();
    displayIdle();
    Preferences preferences;
    preferences.begin("wifi", true);
    const String ssid = preferences.getString("ssid", "");
    const String password = preferences.getString("password", "");
    preferences.end();
    hasWifiCredentials = !ssid.isEmpty();
    if (hasWifiCredentials) {
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false); // Avoid modem-sleep latency while driving the remote LED display.
        WiFi.begin(ssid.c_str(), password.c_str());
        wifiConnectStartedMs = millis();
        Serial.printf("Connecting to saved Wi-Fi SSID: %s\n", ssid.c_str());
    }
    else {
        startProvisioningAccessPoint();
    }
    if (!SPIFFS.begin(false)) Serial.println("SPIFFS unavailable; using built-in Wi-Fi setup page");
    webServer.begin();
    receiver.begin();
    Serial.println("ESP32-C3 remote display ready");
}
void loop() {
    maintainWifi();
    webServer.update();
    receiver.update();
}
