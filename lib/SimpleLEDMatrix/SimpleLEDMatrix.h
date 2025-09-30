#ifndef SIMPLE_LED_MATRIX_H
#define SIMPLE_LED_MATRIX_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Forward declaration of font array
extern const uint8_t font5x7[][5];

class SimpleLEDMatrix {
private:
    Adafruit_NeoPixel strip;
    uint16_t width, height;
    uint8_t brightness;
    uint16_t panelWidth = 16;  // Width of one panel
    uint16_t panelHeight = 16; // Height of one panel
    uint16_t panelsX = 4;      // Number of panels horizontally
    uint16_t panelsY = 1;      // Number of panels vertically
    String lastDisplayedText = ""; // To prevent unnecessary updates
    uint8_t lastR = 255, lastG = 255, lastB = 255; // Last used colors
    
    // Convert X,Y coordinates to linear pixel index for 4-panel layout
    uint16_t xyToIndex(uint16_t x, uint16_t y);
    
public:
    SimpleLEDMatrix(uint8_t pin, uint16_t w, uint16_t h, uint8_t bright = 50);
    
    void begin();
    void clear();
    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
    void show();
    void drawChar(char c, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
    void showTime(unsigned long milliseconds, uint8_t r = 0, uint8_t g = 255, uint8_t b = 0);
    void showText(const String& text, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void setBrightness(uint8_t bright);
};

#endif