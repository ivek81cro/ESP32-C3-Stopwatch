#pragma once

#include <Arduino.h>
#include <WebServer.h>

class RemoteDisplayWebServer {
public:
    void begin();
    void update();
private:
    WebServer _server{80};
    uint32_t _restartAtMs = 0;

    void handleRoot();
    void handleWifiSave();
    void handleStyle();
    String readFileOrFallback(const char* path, const char* fallback) const;
};
