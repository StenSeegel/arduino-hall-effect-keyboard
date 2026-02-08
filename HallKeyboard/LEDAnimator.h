/**
 * LED ANIMATOR LAYER (Visual Effects)
 * 
 */

#ifndef LED_ANIMATOR_H
#define LED_ANIMATOR_H

#include <Adafruit_NeoPixel.h>
#include "ArduinoTapTempo.h"

extern Adafruit_NeoPixel pixels;
extern volatile bool midiClockActive; // From MidiClockReceiver

// ============================================
// LED ANIMATOR STATE
// ============================================

// Confirmation Blink State
int confirmationSwitchIndex = -1;
unsigned long lastBlinkTime = 0;
int blinkCounter = 0;
bool blinkState = false;

// Multi-Note Blink State
unsigned long lastLEDBlinkTime = 0;
bool ledBlinkState = false;
const unsigned long LED_BLINK_INTERVAL = 250;

// Tap Tempo LED State
unsigned long lastTapTempoLEDTime = 0;
bool tapTempoLEDState = false;

// Octave Display State
bool octaveLEDActive = false;
unsigned long octaveLEDStartTime = 0;
const unsigned long OCTAVE_LED_DURATION = 800;

// Error LED State
int errorLEDIndex = -1;
unsigned long errorLEDStartTime = 0;
const unsigned long ERROR_LED_DURATION = 200;

// External variables for animation logic
extern bool isIdle;
extern bool inSubmenu;
extern bool arpeggiatorActive;
extern int bpmPriorityBeats;
extern ArduinoTapTempo tapTempo;
extern bool heldNotes[NUM_SWITCHES];
extern const int ledMapping[NUM_SWITCHES];
extern const bool isBlackKey[NUM_SWITCHES];
extern Button switches[NUM_SWITCHES];

/**
 * Initialisiere
 */
void initLEDAnimator() {
  //Serial.println("LED Animator init");
}

/**
 * Update Error-LED Blinken (non-blocking)
 */
void updateErrorLED() {
  if (errorLEDIndex < 0) return;  // Kein Error-Blink aktiv
  
  unsigned long elapsed = millis() - errorLEDStartTime;
  
  // Beende Error-Blink nach 200ms
  if (elapsed >= ERROR_LED_DURATION) {
    errorLEDIndex = -1;
    ledDirty = true;
    return;
  }
  
  // Blinke LED schnell (50ms an, 50ms aus)
  int ledIndex = 1; // Standard Error LED
  if ((elapsed / 50) % 2 == 0) {
    setLEDColor(ledIndex, COLOR_RED_IDX, 255);
  } else {
    turnOffLED(ledIndex);
  }
}

/**
 * Update LED 8 Blinklogik für Tap Tempo (nur wenn Arpeggiator aktiv)
 * Zeigt auch MIDI Clock Sync Status an
 */
void updateTapTempoLED() {
  if (inSubmenu) {
    tapTempoLEDState = false;
    return;
  }

  bool beatHappened = tapTempo.onBeat();
  if (beatHappened && bpmPriorityBeats > 0) {
    bpmPriorityBeats--;
  }

  // Nur anzeigen wenn Arp aktiv UND (Idle ODER BPM Priority)
  bool canShow = arpeggiatorActive && (isIdle || bpmPriorityBeats > 0);

  if (!canShow) {
    // Falls LED noch vom vorherigen Pulse an war, ausmachen
    if (tapTempoLEDState) {
      turnOffLED(7);
      tapTempoLEDState = false;
    }
    return;
  }

  if (beatHappened) {
    lastTapTempoLEDTime = millis();
    tapTempoLEDState = true;
  }

  unsigned long currentTime = millis();
  unsigned long beatLength = tapTempo.getBeatLength();
  
  // Wenn kein Tempo gesetzt: LED aus
  if (beatLength <= 0 || beatLength > 5000) {
    if (tapTempoLEDState) {
      turnOffLED(7);
      tapTempoLEDState = false;
    }
    return;
  }
  
  const unsigned long PULSE_DURATION = 100;
  unsigned long timeSinceBeat = currentTime - lastTapTempoLEDTime;
  
  if (tapTempoLEDState && timeSinceBeat >= PULSE_DURATION) {
    tapTempoLEDState = false;
    // Wenn wir nicht im Idle-Mode sind, duerfen wir die LED nicht aktiv AUS schalten,
    // da sonst die Note Layer (LEDDisplay.h) geflackert wird.
    if (isIdle) {
      turnOffLED(7);
    }
  }
  
  // Farbwahl: MIDI Clock = Cyan, TapTempo = White
  uint8_t colorIdx = midiClockActive ? COLOR_CYAN_IDX : COLOR_WHITE_IDX;
  
  if (tapTempoLEDState) {
    setLEDColor(7, colorIdx, 255);
  } else {
    // Nur im Idle Mode ausschalten. Im Performance Mode ueberlassen wir das dem Note Layer.
    if (isIdle) {
       turnOffLED(7);
    }
  }
}

/**
 * Multi-Note Blink Logik (wenn mehrere Tasten auf einer LED liegen)
 */
void updateLEDMultiNoteBlink() {
  unsigned long currentTime = millis();
  if (currentTime - lastLEDBlinkTime >= LED_BLINK_INTERVAL) {
    lastLEDBlinkTime = currentTime;
    ledBlinkState = !ledBlinkState;
    
    for (int ledIndex = 0; ledIndex < 8; ledIndex++) {
      int activeCount = 0;
      int activeSwitches[2];
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (ledMapping[i] == ledIndex && switches[i].isDown()) {
          if (activeCount < 2) {
            activeSwitches[activeCount] = i;
          }
          activeCount++;
        }
      }
      
      if (activeCount >= 2) {
        int switchIndex = ledBlinkState ? activeSwitches[0] : activeSwitches[1];
        uint8_t colorIdx = isBlackKey[switchIndex] ? COLOR_PINK_IDX : COLOR_WHITE_IDX;
        setLEDColor(ledIndex, colorIdx, 255);
      }
    }
  }
}

/**
 * Starte Bestätigungs-Blinken für einen Switch (3x Blink)
 */
void confirmLED(int switchIndex) {
  confirmationSwitchIndex = switchIndex;
  blinkCounter = 0;
  blinkState = false;
  lastBlinkTime = millis();
}

/**
 * Update Bestätigungs-Blinken (wird jede Loop aufgerufen)
 */
void updateConfirmBlink() {
  if (confirmationSwitchIndex >= 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 300) {
      lastBlinkTime = currentTime;
      
      int ledIndex = ledMapping[confirmationSwitchIndex];
      
      if (ledIndex < 0) {
        confirmationSwitchIndex = -1;
        return;
      }

      if (blinkState) {
        turnOffLED(ledIndex);
        blinkCounter++;
        if (blinkCounter >= 3) {
          confirmationSwitchIndex = -1;
          return;
        }
      } else {
        uint8_t colorIdx = isBlackKey[confirmationSwitchIndex] ? COLOR_PINK_IDX : COLOR_WHITE_IDX;
        setLEDColor(ledIndex, colorIdx, 255);
      }
      blinkState = !blinkState;
    }
  }
}

/**
 * Update Animationen - Zentrale Steuerung
 */
void updateLEDAnimations() {
  updateConfirmBlink();
  updateErrorLED();
  updateTapTempoLED();
  updateLEDMultiNoteBlink();
}

/**
 * Deaktiviere Controller-LEDs wenn Noten gespielt werden
 */
void disableControllerLEDsForNotes() {
  if (inSubmenu) return;
  
  if (isIdle) {
    isIdle = false;
    pixels.clear();
    pixels.show();
  }
}

/**
 * showOctaveLED() - Visual feedback for octave change
 */
void showOctaveLED(int octave) {
  // Simple visual feedback: blink the LED corresponding to octave (offset by 1)
  confirmLED(octave % NUM_SWITCHES);
}

#endif

