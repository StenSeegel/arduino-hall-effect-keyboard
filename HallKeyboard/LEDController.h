/**
 * LED CONTROLLER LAYER - MINIMAL
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Adafruit_NeoPixel.h>

// ============================================
// LED HARDWARE CONFIGURATION
// ============================================

const int NUM_LEDS = 8;              // 8 WS2812 LEDs
const int LED_PIN = A5;              // Pin A5 als Digital
const int LED_BRIGHTNESS = 55;      // LED Helligkeit (0-255)

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const uint32_t colorPalette[] = {
  0x000000, 0x00FF00, 0xFF0000, 0x0000FF, 0xFFFF00,
  0x00FFFF, 0xFF00FF, 0xFFFFFF, 0x88FF00, 0x00FF88,
};

// Farben als RGB-Hex
const uint32_t WHITE_KEY_COLOR = 0xFFFFFF;   // Weiß für weiße Tasten
const uint32_t BLACK_KEY_COLOR = 0xFF69B4;   // Hot Pink für schwarze Tasten
#define COLOR_OFF_IDX      0
#define COLOR_RED_IDX      1
#define COLOR_GREEN_IDX    2
#define COLOR_BLUE_IDX     3
#define COLOR_YELLOW_IDX   4
#define COLOR_MAGENTA_IDX  5
#define COLOR_CYAN_IDX     6
#define COLOR_WHITE_IDX    7
#define COLOR_ORANGE_IDX   8
#define COLOR_PINK_IDX     9
#define NUM_COLORS         10

uint8_t ledColorIdx[12];
uint8_t ledBrightness[12];
bool ledDirty = false;

void syncLEDStrip() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t color = colorPalette[ledColorIdx[i] % NUM_COLORS];
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    r = (r * ledBrightness[i]) / 255;
    g = (g * ledBrightness[i]) / 255;
    b = (b * ledBrightness[i]) / 255;
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
  ledDirty = false;
}

void initLEDController() {
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);
  for (int i = 0; i < NUM_LEDS; i++) {
    ledColorIdx[i] = COLOR_OFF_IDX;
    ledBrightness[i] = 0;
  }
  pixels.clear();
  pixels.show();
  ledDirty = false;
}

void setLEDColor(int idx, uint8_t colorIdx, uint8_t brightness = 200) {
  if (idx < 0 || idx >= NUM_LEDS) return;
  ledColorIdx[idx] = colorIdx;
  ledBrightness[idx] = brightness;
  ledDirty = true;
}

void turnOffLED(int idx) {
  if (idx < 0 || idx >= NUM_LEDS) return;
  ledColorIdx[idx] = COLOR_OFF_IDX;
  ledBrightness[idx] = 0;
  ledDirty = true;
}

void turnOffAllLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    turnOffLED(i);
  }
}

uint8_t getLEDColorIdx(int idx) {
  if (idx < 0 || idx >= NUM_LEDS) return COLOR_OFF_IDX;
  return ledColorIdx[idx];
}

extern const int ledMapping[13];
extern const bool isBlackKey[13];

/**
 * setLED() - High-level implementation for switch-based LED control
 */
void setLED(int switchIndex, bool on, bool skipLEDs = false) {
  if (switchIndex < 0 || switchIndex >= 13) return;
  if (skipLEDs) return;
  
  int ledIndex = ledMapping[switchIndex];
  if (ledIndex < 0 || ledIndex >= NUM_LEDS) return;
  
  if (on) {
    uint8_t colorIdx = isBlackKey[switchIndex] ? COLOR_PINK_IDX : COLOR_WHITE_IDX;
    setLEDColor(ledIndex, colorIdx, 255);
  } else {
    turnOffLED(ledIndex);
  }
}

#endif
