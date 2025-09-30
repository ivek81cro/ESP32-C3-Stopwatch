#include "SimpleLEDMatrix.h"

// Constructor implementation
SimpleLEDMatrix::SimpleLEDMatrix(uint8_t pin, uint16_t w, uint16_t h, uint8_t bright)
    : strip(w * h, pin, NEO_GRB + NEO_KHZ800), width(w), height(h), brightness(bright) {
}

// Matrix font definition moved from header
const uint8_t font5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // : (colon)
    {0x00, 0x00, 0x60, 0x60, 0x00}, // . (dot/period)
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
};

// Convert X,Y coordinates to linear pixel index for 4-panel layout
uint16_t SimpleLEDMatrix::xyToIndex(uint16_t x, uint16_t y) {
    if (x >= width || y >= height) return 0;
    
    // Mirror X coordinate to fix reversed display
    x = width - 1 - x;
    
    // Determine which panel we're in
    uint16_t panelX = x / panelWidth;
    uint16_t panelY = y / panelHeight;
    
    // Local coordinates within the panel
    uint16_t localX = x % panelWidth;
    uint16_t localY = y % panelHeight;
    
    // Calculate base pixel index for this panel
    uint16_t panelIndex = panelY * panelsX + panelX;
    uint16_t panelBasePixel = panelIndex * panelWidth * panelHeight;
    
    // Within each 16x16 panel, map coordinates to pixel index
    // Assuming serpentine/zigzag pattern within panel
    uint16_t pixelInPanel;
    if (localY % 2 == 0) {
        // Even rows: left to right
        pixelInPanel = localY * panelWidth + localX;
    } else {
        // Odd rows: right to left
        pixelInPanel = localY * panelWidth + (panelWidth - 1 - localX);
    }
    
    return panelBasePixel + pixelInPanel;
}

void SimpleLEDMatrix::begin() {
    strip.begin();
    strip.setBrightness(brightness);
    strip.clear();
    strip.show();
}

void SimpleLEDMatrix::clear() {
    strip.clear();
    strip.show();
    lastDisplayedText = ""; // Reset cache when clearing
    lastR = 255; lastG = 255; lastB = 255; // Reset color cache
}

void SimpleLEDMatrix::setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= width || y >= height) return;
    uint16_t pixel = xyToIndex(x, y);
    strip.setPixelColor(pixel, strip.Color(r, g, b));
}

void SimpleLEDMatrix::show() {
    strip.show();
}

void SimpleLEDMatrix::drawChar(char c, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t index = 12; // space by default
    
    if (c >= '0' && c <= '9') {
        index = c - '0';
    } else if (c == ':') {
        index = 10;
    } else if (c == '.') {
        index = 11;
    }
    
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[index][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void SimpleLEDMatrix::showTime(unsigned long milliseconds, uint8_t r, uint8_t g, uint8_t b) {
    // Always show MM:SS.mmm format (minutes:seconds.milliseconds)
    uint8_t minutes = (milliseconds / 60000) % 60;
    uint8_t seconds = (milliseconds / 1000) % 60;
    uint16_t ms = milliseconds % 1000;
    
    char timeStr[16];
    sprintf(timeStr, "%02d:%02d.%03d", minutes, seconds, ms);
    
    // Only update if time display AND colors have actually changed
    String currentTimeStr = String(timeStr);
    if (currentTimeStr == lastDisplayedText && r == lastR && g == lastG && b == lastB) {
        return;
    }
    
    lastDisplayedText = currentTimeStr;
    lastR = r; lastG = g; lastB = b;
    
    // Clear only the pixels we need to update to reduce flicker
    strip.clear();
    
    // Display time string centered on the matrix
    // Calculate total width: 10 chars * 6px spacing - 1px (no gap after last char) = 59px
    uint16_t totalWidth = 59; // 10 * 6 - 1
    uint16_t startX = (width - totalWidth) / 2 + 3; // Center horizontally + offset right
    
    for (int i = 0; timeStr[i] != '\0' && i < 10; i++) {
        drawChar(timeStr[i], startX + i * 6, 4, r, g, b);  // 6px spacing = 5px char + 1px gap
    }
    
    show();
}

void SimpleLEDMatrix::showText(const String& text, uint8_t r, uint8_t g, uint8_t b) {
    // Only update if text AND colors have changed
    if (text == lastDisplayedText && r == lastR && g == lastG && b == lastB) {
        return;
    }
    
    lastDisplayedText = text;
    lastR = r; lastG = g; lastB = b;
    
    clear();
    
    // Center the text on the matrix
    uint16_t textWidth = text.length() * 6 - 1; // text length * 6px - 1px gap
    uint16_t startX = (width - textWidth) / 2 + 3; // Center + offset right
    
    for (int i = 0; i < text.length() && i < 10; i++) {  // Max 10 characters for 64px width
        drawChar(text[i], startX + i * 6, 5, r, g, b);  // 6px spacing with centering
    }
    
    show();
}

void SimpleLEDMatrix::setBrightness(uint8_t bright) {
    brightness = bright;
    strip.setBrightness(brightness);
}