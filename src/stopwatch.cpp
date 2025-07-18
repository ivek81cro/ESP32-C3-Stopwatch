#include "stopwatch.h"

// LED Segment Mapping for 7-Segment Displays
const int Stopwatch::digitSegments[6][7] = {
    {0, 8, 16, 24, 32, 40, 48}, 
    {56, 64, 72, 80, 88, 96, 104},
    {112, 120, 128, 136, 144, 152, 160},
    {168, 176, 184, 192, 200, 208, 216},
    {224, 232, 240, 248, 256, 264, 272},
    {280, 288, 296, 304, 312, 320, 328}
};

// Global Variables
CRGB Stopwatch::leds[NUM_LEDS];
DataPacket Stopwatch::sendData = {};
DataPacket Stopwatch::receivedData = {};
//{0x10, 0x00, 0x3B, 0x00, 0x4E, 0xE4}
//{0x7C, 0x2C, 0x67, 0xD3, 0x0E, 0x60}
//{0xA0, 0x85, 0xE3, 0x4E, 0x56, 0x20}
uint8_t Stopwatch::receiverMAC[] = {0x7C, 0x2C, 0x67, 0xD3, 0x0E, 0x60};
bool Stopwatch::triggerArmed = false;
bool Stopwatch::timerRunning = false;
//unsigned long Stopwatch::startTime = 0;
//unsigned long Stopwatch::elapsedTime = 0;
unsigned long Stopwatch::lastDisarmTime = 0;
//unsigned long Stopwatch::stopTime = 0;

void Stopwatch::setup() {
    // Initialize Serial, FastLED, and pins
    Serial.begin(115200);
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(255);
    FastLED.clear();
    pinMode(LASER_PIN, INPUT_PULLUP);

    DEBUG_PRINTLN("Stopwatch initialized");

    // Initialize WiFi and ESP-NOW
    WiFi.mode(WIFI_STA);
    delay(5000);
    DEBUG_PRINTLN("STA MAC Address: " + WiFi.macAddress());
    initializeESPNow();
}

void Stopwatch::loop() {
    handleLaserTrigger();
}

void Stopwatch::initializeESPNow() {
    if (esp_now_init() != ESP_OK) {
        DEBUG_PRINTLN("ESP-NOW initialization failed!");
        return;
    }

    esp_now_register_recv_cb(onReceive);
    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        DEBUG_PRINTLN("Failed to add ESP-NOW peer");
    }
}

Stopwatch::~Stopwatch() {
    esp_now_deinit();
}

void Stopwatch::handleLaserTrigger() {
    bool laserTripped = (digitalRead(LASER_PIN) == HIGH);

    if (triggerArmed) {
        if (laserTripped) {
            if (!timerRunning) {  // Start stopwatch
                sendData.startTime = millis();
                timerRunning = true;
                lastDisarmTime = sendData.startTime;
                triggerArmed = false;
                sendData.code = 9; // code for stopwatch started
                DEBUG_PRINTF("Stopwatch started, startTime: %lu\n", sendData.startTime);
                Stopwatch::getInstance().sendDataToStopwatch();
            } else {  // Stop stopwatch
                sendData.stopTime = millis();
                sendData.elapsedTime = millis() - sendData.startTime;
                sendData.code = 8; // code for stopwatch stopped
                timerRunning = false;
                triggerArmed = false;
                updateTimeDisplay(sendData.elapsedTime);
                DEBUG_PRINTF("Stop timestamp: %d ms\n Elapsed time: %d ms\n",
                    sendData.stopTime, sendData.elapsedTime);

                // Send elapsed time
                Stopwatch::getInstance().sendDataToStopwatch();
            }
        }
    }
    if (!triggerArmed && timerRunning && millis() - lastDisarmTime >= DISARM_TIME) {
        //triggerArmed = true;
    }
    if (timerRunning) {
        updateTimeDisplay(millis() - sendData.startTime);
    }
}

void Stopwatch::sendDataToStopwatch()
{
    esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&sendData, sizeof(sendData));
    DEBUG_PRINTLN("Sending data result...:");
    DEBUG_PRINTLN(result);
}

void Stopwatch::updateTimeDisplay(unsigned long time, int code) {
    uint8_t minutes = (time / 60000) % 60;
    uint8_t seconds = (time / 1000) % 60;
    uint8_t centiseconds = (time % 1000) / 10;

    clearAndLightDigit(0, minutes / 10, code);
    clearAndLightDigit(1, minutes % 10, code);
    clearAndLightDigit(2, seconds / 10, code);
    clearAndLightDigit(3, seconds % 10, code);
    clearAndLightDigit(4, centiseconds / 10, code);
    clearAndLightDigit(5, centiseconds % 10, code);

    DEBUG_PRINTF("%02d:%02d:%02d\n", minutes, seconds, centiseconds);
}

void Stopwatch::clearAndLightDigit(int digit, int number, int code) {
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
                leds[startIndex + j] = code == 5 ? CRGB::Green : CRGB::Red;
            }
            segments++;
        }
    }
    FastLED.show(); 
}

void Stopwatch::onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
    Stopwatch& instance = Stopwatch::getInstance();
    const DataPacket* packet = reinterpret_cast<const DataPacket*>(incomingData);
    if (instance.timerRunning && packet->code != 10) {
        DEBUG_PRINTLN("Receive blocked");
        instance.sendData.code = 5;//code for message recieve blocked
        esp_now_send(instance.receiverMAC, (uint8_t *)&instance.sendData, sizeof(instance.sendData));
        instance.sendData.code = 0;
    } else {
        memcpy(&instance.receivedData, incomingData, sizeof(instance.receivedData));
        DEBUG_PRINTF("Received: %d ms\n Code: %d\n", instance.receivedData.elapsedTime, instance.receivedData.code);
        instance.updateTimeDisplay(instance.receivedData.elapsedTime, instance.receivedData.code);
        DEBUG_PRINTLN(instance.receivedData.code);
        instance.manageTrigger();
    }
}

void Stopwatch::onSent(const uint8_t *macAddr, esp_now_send_status_t status) {
    DEBUG_PRINTF("Delivery Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    if (status != ESP_NOW_SEND_SUCCESS) {
        Stopwatch::getInstance().sendDataToStopwatch();
    }
}

void Stopwatch::manageTrigger() {
    if (receivedData.code == 3) { // code for arm trigger
        triggerArmed = true;
        DEBUG_PRINTLN("Trigger armed");
    }
    else if (receivedData.code == 10 ){
        triggerArmed = !triggerArmed;
        DEBUG_PRINTF("Trigger toggled: %s\n", triggerArmed ? "armed" : "disarmed");
    } else {
        triggerArmed = false;
        DEBUG_PRINTLN("Trigger disarmed");
    }
}