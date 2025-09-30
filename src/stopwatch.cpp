#include "stopwatch.h"

// Global Variables
SimpleLEDMatrix* Stopwatch::matrix = nullptr;
DataPacket Stopwatch::sendData = {};
DataPacket Stopwatch::receivedData = {};
uint8_t Stopwatch::receiverMAC[] = {0x7C, 0x2C, 0x67, 0xD3, 0x0E, 0x60};
bool Stopwatch::triggerArmed = false;
bool Stopwatch::timerRunning = false;
unsigned long Stopwatch::lastDisplayUpdate = 0;

void Stopwatch::setup() {
    // Initialize Serial and pins
    Serial.begin(115200);
    pinMode(LASER_PIN, INPUT_PULLUP);

    // Initialize LED Matrix
    matrix = new SimpleLEDMatrix(DATA_PIN, MATRIX_WIDTH, MATRIX_HEIGHT, LED_BRIGHTNESS);
    matrix->begin();
    
    // Show initial time display
    updateTimeDisplay(0);
    DEBUG_PRINTLN("Stopwatch initialized");

    // Initialize WiFi and ESP-NOW
    WiFi.mode(WIFI_STA);
    delay(2000); // Reduced delay
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
    if (matrix) {
        delete matrix;
        matrix = nullptr;
    }
}

void Stopwatch::handleLaserTrigger() {
    bool laserTripped = (digitalRead(LASER_PIN) == HIGH);

    if (triggerArmed) {
        if (laserTripped) {
            if (!timerRunning) {  // Start stopwatch
                sendData.startTime = millis();
                timerRunning = true;
                triggerArmed = false; // Trigger is disarmed when timer starts
                sendData.code = TIMER_STARTED;
                DEBUG_PRINTF("Stopwatch started, startTime: %lu\n", sendData.startTime);
                Stopwatch::getInstance().sendDataToStopwatch();
            } else {  // Stop stopwatch
                sendData.stopTime = millis();
                sendData.elapsedTime = millis() - sendData.startTime;
                sendData.code = TIMER_STOPPED;
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
    
    // No automatic re-arming - wait for user input to toggle trigger
    
    if (timerRunning) {
        // Update display every 10ms for smooth refresh
        unsigned long currentTime = millis();
        if (currentTime - lastDisplayUpdate >= 10) {
            updateTimeDisplay(currentTime - sendData.startTime);
            lastDisplayUpdate = currentTime;
        }
    } else {
        // When timer is not running, show the last elapsed time or 0
        unsigned long currentTime = millis();
        if (currentTime - lastDisplayUpdate >= 100) { // Update less frequently when stopped
            updateTimeDisplay(sendData.elapsedTime);
            lastDisplayUpdate = currentTime;
        }
    }
}

void Stopwatch::sendDataToStopwatch()
{
    esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&sendData, sizeof(sendData));
    DEBUG_PRINTLN("Sending data result...:");
    DEBUG_PRINTLN(result);
}

void Stopwatch::updateTimeDisplay(unsigned long time, int code) {
    if (!matrix) return;
    
    // Determine color based on code
    uint8_t r = 255, g = 255, b = 255; // Default white
    
    switch (code) {
        case MSG_BLOCKED:
            r = 0; g = 255; b = 0;  // Green
            break;
        case TIMER_STOPPED:
        case TIMER_STARTED:
            r = 255; g = 0; b = 0;  // Red
            break;
        default:
            if (timerRunning) {
                r = 255; g = 255; b = 0;  // Yellow for running timer
            } else {
                r = 0; g = 255; b = 0;    // Green for stopped timer
            }
            break;
    }
    
    // Use the built-in timer display function
    matrix->showTime(time, r, g, b);
    

}



void Stopwatch::onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
    Stopwatch& instance = Stopwatch::getInstance();
    const DataPacket* packet = reinterpret_cast<const DataPacket*>(incomingData);
    
    if (instance.timerRunning && packet->code != TOGGLE_TRIGGER) {
        DEBUG_PRINTLN("Receive blocked");
        instance.sendData.code = MSG_BLOCKED; // code for message receive blocked
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
    if (receivedData.code == 3) { // code for arm trigger (legacy)
        triggerArmed = true;
        DEBUG_PRINTLN("Trigger armed");
        if (matrix) {
            matrix->showText("ARMED", 255, 255, 0); // Yellow text
        }
    }
    else if (receivedData.code == TIMER_RESET) {
        // Complete system reset
        triggerArmed = false;
        timerRunning = false;
        sendData.elapsedTime = 0;
        sendData.startTime = 0;
        sendData.stopTime = 0;
        sendData.code = TRIGGER_DISARMED;
        lastDisplayUpdate = 0;
        
        if (matrix) {
            matrix->clear();
            updateTimeDisplay(0);
        }
        
        sendDataToStopwatch();
        DEBUG_PRINTLN("Timer reset, trigger disarmed");
    }
    else if (receivedData.code == TOGGLE_TRIGGER) {
        triggerArmed = !triggerArmed;
        sendData.code = triggerArmed ? TRIGGER_ARMED : TRIGGER_DISARMED;
        sendDataToStopwatch();
        DEBUG_PRINTF("Trigger toggled: %s\n", triggerArmed ? "armed" : "disarmed");
        if (matrix) {
            matrix->showText(triggerArmed ? "ARMED" : "DISARM",
                           triggerArmed ? 255 : 255, triggerArmed ? 255 : 0, triggerArmed ? 0 : 0);
        }
    } else {
        triggerArmed = false;
        DEBUG_PRINTLN("Trigger disarmed");
        if (matrix) {
            matrix->showText("DISARM", 255, 0, 0); // Red text
        }
    }
}

