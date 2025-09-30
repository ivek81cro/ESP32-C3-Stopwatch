#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "SimpleLEDMatrix.h"

// Debugging macros
#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

// Constants
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 16
#define DATA_PIN 21
#define LASER_PIN 4
#define LED_BRIGHTNESS 50

// Communication codes
enum CommCodes {
    MSG_BLOCKED = 5,     // Receive blocked, stopwatch running
    TIMER_RESET = 6,     // Trigger reset
    TIMER_STOPPED = 8,   // Stopwatch stopped
    TIMER_STARTED = 9,   // Stopwatch started
    TOGGLE_TRIGGER = 10, // Toggle trigger state
    TRIGGER_ARMED = 20,  // Trigger armed confirmation
    TRIGGER_DISARMED = 21 // Trigger disarmed confirmation
};

struct DataPacket {
    uint8_t id;
    uint8_t code;
    int stopTime;
    int startTime;
    int elapsedTime;
};

class Stopwatch {
public:
    static Stopwatch& getInstance() {
        static Stopwatch instance;
        return instance;
    }

    void setup();
    void loop();
    ~Stopwatch(); // Destructor

private:
    static SimpleLEDMatrix* matrix;
    static DataPacket sendData;
    static DataPacket receivedData;
    static uint8_t receiverMAC[];
    static bool triggerArmed;
    static bool timerRunning;
    static unsigned long lastDisplayUpdate;

    Stopwatch() {} // Private constructor for singleton pattern
    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;

    void initializeESPNow();
    void handleLaserTrigger();
    void sendDataToStopwatch();
    void updateTimeDisplay(unsigned long time, int code = 0);
    static void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len);
    static void onSent(const uint8_t *macAddr, esp_now_send_status_t status);
    void manageTrigger();
};

#endif