#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

// Constants
#define NUM_LEDS 336
#define DATA_PIN 21
#define LASER_PIN 4
#define DISARM_TIME 10000
#define SEGMENT_LENGTH 8

// Data Structures
struct DataPacket {
    uint8_t seconds;
    int elapsedTime;
};

// Global Variables
extern CRGB leds[NUM_LEDS];
extern DataPacket sendData;
extern DataPacket receivedData;
extern uint8_t receiverMAC[];
extern bool triggerArmed;
extern bool timerRunning;
extern unsigned long startTime;
extern unsigned long elapsedTime;
extern unsigned long lastDisarmTime;

// Function Prototypes
void initializeESPNow();
void handleLaserTrigger();
void updateTimeDisplay(unsigned long time);
void clearAndLightDigit(int digit, int number);
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len);
void onSent(const uint8_t *macAddr, esp_now_send_status_t status);

#endif
