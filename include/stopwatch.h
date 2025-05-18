#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Arduino.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

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
#define NUM_LEDS 336
#define DATA_PIN 21
#define LASER_PIN 4
#define DISARM_TIME 10000
#define SEGMENT_LENGTH 8

// Data Structures
/*Codes:    5 = recieve blocked, stopwatch running
            9 = stopwatch started, start timestamp
            8 = stopwatch stopped, stop timestamp
            6 = trigger reset
*/
struct DataPacket {
    uint8_t code; //code for messages between MCU's
    int stopTime; //elapsed time in milliseconds
    int startTime; //start timestamp
    int elapsedTime; //elapsed time in milliseconds
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
    static const int digitSegments[6][7];
    static CRGB leds[NUM_LEDS];
    static DataPacket sendData;
    static DataPacket receivedData;
    static uint8_t receiverMAC[];
    static bool triggerArmed;
    static bool timerRunning;
    //static unsigned long stopTime;
    //static unsigned long startTime;
    //static unsigned long elapsedTime;
    static unsigned long lastDisarmTime;

    Stopwatch() {} // Private constructor for singleton pattern
    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;

    void initializeESPNow();
    void handleLaserTrigger();
    void sendDataToStopwatch();
    void updateTimeDisplay(unsigned long time, int code = 0);
    void clearAndLightDigit(int digit, int number, int code = 0);
    static void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len);
    static void onSent(const uint8_t *macAddr, esp_now_send_status_t status);
    void manageTrigger();
};

#endif