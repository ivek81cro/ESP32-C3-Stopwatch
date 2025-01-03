#include "main.h"

// LED Segment Mapping for 7-Segment Displays
const int digitSegments[6][7] = {
    {0, 8, 16, 24, 32, 40, 48}, 
    {56, 64, 72, 80, 88, 96, 104},
    {112, 120, 128, 136, 144, 152, 160},
    {168, 176, 184, 192, 200, 208, 216},
    {224, 232, 240, 248, 256, 264, 272},
    {280, 288, 296, 304, 312, 320, 328}
};

// Global Variables
CRGB leds[NUM_LEDS];
DataPacket sendData = {};
DataPacket receivedData = {};
uint8_t receiverMAC[] = {0x7C, 0x2C, 0x67, 0xD3, 0x0E, 0x60};
bool triggerArmed = true;
bool timerRunning = false;
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long lastDisarmTime = 0;

void setup() {
    // Initialize Serial, FastLED, and pins
    Serial.begin(115200);
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(128);
    FastLED.clear();
    pinMode(LASER_PIN, INPUT_PULLUP);

    Serial.println("Stopwatch initialized");

    // Initialize WiFi and ESP-NOW
    WiFi.mode(WIFI_STA);
    initializeESPNow();
}

void loop() {
    handleLaserTrigger();
    if (!triggerArmed && timerRunning) {
        updateTimeDisplay(millis() - startTime);
    }
}

void initializeESPNow() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initialization failed!");
        return;
    }

    esp_now_register_recv_cb(onReceive);
    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer");
    }
}

void handleLaserTrigger() {
    bool laserTripped = (digitalRead(LASER_PIN) == HIGH);

    if (triggerArmed) {
        if (laserTripped) {
            if (!timerRunning) {  // Start stopwatch
                startTime = millis();
                timerRunning = true;
                lastDisarmTime = startTime;
                triggerArmed = false;
            } else {  // Stop stopwatch
                elapsedTime = millis() - startTime;
                timerRunning = false;
                updateTimeDisplay(elapsedTime);
                Serial.printf("Elapsed time: %d ms\n", elapsedTime);

                // Send elapsed time
                sendData.elapsedTime = elapsedTime;
                esp_now_send(receiverMAC, (uint8_t *)&sendData, sizeof(sendData));
            }
        } else if (timerRunning) {
            updateTimeDisplay(millis() - startTime);
        }
    } else if (!timerRunning && millis() - lastDisarmTime >= DISARM_TIME) {
        triggerArmed = true;
    }
}

void updateTimeDisplay(unsigned long time) {
    uint8_t minutes = (time / 60000) % 60;
    uint8_t seconds = (time / 1000) % 60;
    uint8_t centiseconds = (time % 1000) / 10;

    clearAndLightDigit(0, minutes / 10);
    clearAndLightDigit(1, minutes % 10);
    clearAndLightDigit(2, seconds / 10);
    clearAndLightDigit(3, seconds % 10);
    clearAndLightDigit(4, centiseconds / 10);
    clearAndLightDigit(5, centiseconds % 10);

    Serial.printf("%02d:%02d:%02d\n", minutes, seconds, centiseconds);
}

void clearAndLightDigit(int digit, int number) {
    const char *segmentPatterns[] = {
        "abcefg", "cg", "abdfg", "bcdfg", "cdeg", 
        "bcdef", "abcdef", "cfg", "abcdefg", "bcdefg"
    };

    for (int i = 0; i < 7; i++) {  // Clear all segments
        int startIndex = digitSegments[digit][i];
        for (int j = 0; j < SEGMENT_LENGTH; j++) {
            leds[startIndex + j] = CRGB::Black;
        }
    }

    if (number >= 0 && number <= 9) {  // Light specified segments
        const char *segments = segmentPatterns[number];
        while (*segments) {
            int segmentIndex = *segments - 'a';
            int startIndex = digitSegments[digit][segmentIndex];
            for (int j = 0; j < SEGMENT_LENGTH; j++) {
                leds[startIndex + j] = CRGB::Green;
            }
            segments++;
        }
    }

    FastLED.show();
}

void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (timerRunning) {
        Serial.println("Receive blocked");
        sendData.seconds = 5;
        esp_now_send(receiverMAC, (uint8_t *)&sendData, sizeof(sendData));
        sendData.seconds = 0;
        return;
    }

    memcpy(&receivedData, incomingData, sizeof(receivedData));
    Serial.printf("Received: %d ms\n", receivedData.elapsedTime);
    updateTimeDisplay(receivedData.elapsedTime);

    if (receivedData.seconds == 6) {
        triggerArmed = true;
        Serial.println("Trigger re-armed");
    }
}

void onSent(const uint8_t *macAddr, esp_now_send_status_t status) {
    Serial.printf("Delivery Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}
