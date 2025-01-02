#include <Arduino.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

#define NUM_LEDS 336             // Total number of LEDs
#define DATA_PIN 21              // Data pin connected to the LEDs
#define LASER_PIN 4             // Laser sensor pin
#define DISARM_TIME 10000        // Disarm time in milliseconds (10 seconds)
#define SEGMENT_LENGTH 8         // Length of each LED segment

// Structure to receive data
typedef struct {
  uint8_t seconds;
  int elapsedTime;
} DataPacket;

DataPacket sendData;
DataPacket receivedData;

CRGB leds[NUM_LEDS];

// MAC address of the Receiver ESP32
uint8_t receiverMAC[] = {0x7C, 0x2C, 0x67, 0xD3, 0x0E, 0x60};

// Trigger and timer variables
bool triggerArmed = true;
bool timerRunning = false;
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long lastDisarmTime = 0;

// Segment LED indices for each digit
const int digitSegments[6][7] = {
  {0, 8, 16, 24, 32, 40, 48},    // Digit 1
  {56, 64, 72, 80, 88, 96, 104}, // Digit 2
  {112, 120, 128, 136, 144, 152, 160}, // Digit 3
  {168, 176, 184, 192, 200, 208, 216}, // Digit 4
  {224, 232, 240, 248, 256, 264, 272}, // Digit 5
  {280, 288, 296, 304, 312, 320, 328}  // Digit 6
};

// Function prototypes
void clearSegment(int digit, int segment);
void lightSegments(int digit, const char* segments);
void displayTime(unsigned long time);
void displayDigit(int digit, int number);
uint8_t *getTime(unsigned long time);

void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (timerRunning) {
        // Ignore incoming data while blocking
        Serial.println("Receive blocked");
        return;
    }

    memcpy(&receivedData, incomingData, sizeof(receivedData));
    Serial.print("Received from Device B: ");
    Serial.printf("%d \n", receivedData.elapsedTime);
    displayTime(receivedData.elapsedTime);
}

void onSent(const uint8_t *macAddr, esp_now_send_status_t status) {
    Serial.print("Delivery Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  
  pinMode(LASER_PIN, INPUT_PULLUP);
  delay(2000);
  Serial.begin(115200);
  delay(2000);
  Serial.println("Stopwatch started");

  WiFi.mode(WIFI_STA);

  String macAddress = WiFi.macAddress();
    Serial.println("STA MAC Address: " + macAddress);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    delay(5000);

    esp_now_register_recv_cb(onReceive);
    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

void loop() {
  bool laserTripped = (digitalRead(LASER_PIN) == HIGH);
  
  if (triggerArmed) {
    //Start stopwatch
    if (laserTripped && !timerRunning) {
      startTime = millis();
      timerRunning = true;
      triggerArmed = false;
      lastDisarmTime = millis();
    }
    //Stop stopwatch 
    else if (laserTripped && timerRunning) {
      elapsedTime = millis() - startTime;
      timerRunning = false;
      displayTime(elapsedTime);
      triggerArmed = false;
      Serial.printf("Elapsed time %d",elapsedTime);

      sendData.elapsedTime = elapsedTime;
      delay(3000);
      // Send data
      esp_now_send(receiverMAC, (uint8_t *)&sendData, sizeof(sendData));
      delay(3000);
    }
    //Stopwatch running with posibility to stop
    else if (timerRunning) {
      elapsedTime = millis() - startTime;
      displayTime(elapsedTime);
    }
  }
  //Arming laser sensor timer 
  else if (millis() - lastDisarmTime >= DISARM_TIME) {
    triggerArmed = true;  // Re-arm trigger after disarm time
  }
  //Stopwatch running without posibility to stop 
  if (!triggerArmed && timerRunning) {
    elapsedTime = millis() - startTime;
    displayTime(elapsedTime);
  }
  else {
    elapsedTime = millis() - startTime;
  }
}
//Clear 7 segment number display
void clearSegment(int digit, int segment) {
  int startIndex = digitSegments[digit][segment];
  for (int i = startIndex; i < startIndex + SEGMENT_LENGTH; i++) {
    leds[i] = CRGB::Black;
  }
}
//Light 7 segment display in specified config
void lightSegments(int digit, const char* segments) {
  while (*segments) {
    int segmentIndex = *segments - 'a';
    int startIndex = digitSegments[digit][segmentIndex];
    for (int i = startIndex; i < startIndex + SEGMENT_LENGTH; i++) {
      leds[i] = CRGB::Green;
    }
    segments++;
  }
}
//Light specific LED's on 7 segment display to show number
void displayDigit(int digit, int number) {
  // Clear all segments of the digit first
  for (int i = 0; i < 7; i++) {
    clearSegment(digit, i);
  }

  // Light up the segments according to the number 
  const char* segmentPatterns[] = {
    "abcefg", "cg", "abdfg", "bcdfg", "cdeg", "bcdef",
    "abcdef", "cfg", "abcdefg", "bcdefg"
  };

  if (number >= 0 && number <= 9) {
    lightSegments(digit, segmentPatterns[number]);
  }

  FastLED.show();
}
//Display current time on display
void displayTime(unsigned long time) {
  uint8_t* timeState = getTime(time);

  displayDigit(0, timeState[0] / 10);      // Tens digit of minutes
  displayDigit(1, timeState[0] % 10);      // Units digit of minutes
  displayDigit(2, timeState[1] / 10);      // Tens digit of seconds
  displayDigit(3, timeState[1] % 10);      // Units digit of seconds
  displayDigit(4, timeState[2] / 10); // Tens digit of milliseconds
  displayDigit(5, timeState[2] % 10); // Units digit of milliseconds

  Serial.printf("%02d:%02d:%02d\n", timeState[0], timeState[1], timeState[2]);

  free(timeState);
}

uint8_t* getTime(unsigned long time){
  uint8_t* timeState = (uint8_t*)malloc(3 * sizeof(uint8_t)); 
  timeState[0] = (time / 60000) % 60;
  timeState[1] = (time / 1000) % 60;
  timeState[2] = (time % 1000) / 10;

  return timeState;
}
