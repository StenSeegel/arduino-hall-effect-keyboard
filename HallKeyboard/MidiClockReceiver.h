/**
 * MIDI CLOCK RECEIVER LAYER
 * 
 * Empfängt MIDI Clock Input (24 PPQN):
 * - Lightweight Implementation ohne MIDI Library
 * - Nur System Realtime Messages (0xF8-0xFF)
 * - Synchronisiert HallKeyboard mit externem MIDI Clock
 * - Auto-Detection via Timeout
 * - Fallback zu TapTempo bei Clock Timeout
 * 
 * INPUT:
 *   - MIDI Clock Messages (0xF8) via Serial1
 *   - MIDI Start/Stop/Continue (0xFA/0xFC/0xFB)
 * 
 * OUTPUT:
 *   - midiClockActive (bool)
 *   - calculatedBPM (uint16_t)
 */

#ifndef MIDI_CLOCK_RECEIVER_H
#define MIDI_CLOCK_RECEIVER_H

#include "ArduinoTapTempo.h"

// ============================================
// EXTERNAL VARIABLES & FUNCTIONS
// ============================================
extern ArduinoTapTempo tapTempo;
extern void syncMidiClockPhase();
extern void handleExternalClockPulse();
extern void resetArpeggiatorPhase();

// ============================================
// MIDI CLOCK RECEIVER STATE
// ============================================

bool midiClockActive = false;
unsigned long lastMidiClockMicros = 0;
uint16_t calculatedBPM = 120; // uint16_t statt float spart 2 bytes

// Timeout Configuration
#define MIDI_CLOCK_TIMEOUT_MICROS 500000UL  // erhöht auf 500ms für mehr Stabilität (ca. 12 Pulses bei 30 BPM)

// MIDI System Realtime Messages (1 byte, kein Status-Byte-Logik nötig)
#define MIDI_CLOCK_MSG      0xF8
#define MIDI_START_MSG      0xFA
#define MIDI_CONTINUE_MSG   0xFB
#define MIDI_STOP_MSG       0xFC

// ============================================
// LIGHTWEIGHT MIDI CLOCK HANDLERS
// ============================================

/**
 * Handler für MIDI Clock Pulse (0xF8)
 */
inline void processMidiClock() {
  unsigned long currentMicros = micros();
  
  // BPM Berechnung nur alle 24 Pulses (einmal pro Beat)
  // Optimiert: Verwendet statische lokale Variable statt globaler
  static unsigned long lastBeatMicros = 0;
  static uint8_t pulseCount = 0;
  static bool firstBPMCalculation = true;
  
  pulseCount++;
  if (pulseCount >= 24) {
    pulseCount = 0;
    
    if (lastBeatMicros > 0) {
      unsigned long beatInterval = currentMicros - lastBeatMicros;
      
      // BPM = 60.000.000 µs / beatInterval
      // Vereinfachte Integer-Berechnung spart RAM
      if (beatInterval > 0) {
        uint16_t newBPM = 60000000UL / beatInterval;
        
        // Sanity Check: BPM Limits (30-300 BPM)
        if (newBPM >= 30 && newBPM <= 300) {
          calculatedBPM = newBPM;
        }
      }
    }
    
    lastBeatMicros = currentMicros;
  }
  
  lastMidiClockMicros = currentMicros;
  midiClockActive = true;
  
  // Update globale Taktphase direkt über den Pulse (verhindert Drift)
  handleExternalClockPulse();
}

/**
 * Handler für MIDI Start (0xFA)
 */
inline void processMidiStart() {
  lastMidiClockMicros = micros();
  midiClockActive = true;
  syncMidiClockPhase();
  resetArpeggiatorPhase();
  tapTempo.resetTapChain(); // Interne Phase des TapTempos (für LEDs etc.) resetten
}

/**
 * Handler für MIDI Continue (0xFB)
 */
inline void processMidiContinue() {
  lastMidiClockMicros = micros();
  midiClockActive = true;
  syncMidiClockPhase();
  resetArpeggiatorPhase();
  tapTempo.resetTapChain();
}

/**
 * Handler für MIDI Stop (0xFC)
 */
inline void processMidiStop() {
  // Clock wird nur per Timeout deaktiviert
}

// ============================================
// MIDI CLOCK RECEIVER UPDATE
// ============================================

/**
 * Initialisiert MIDI Clock Receiver
 */
void initMidiClockReceiver() {
  midiClockActive = false;
  lastMidiClockMicros = 0;
  calculatedBPM = 120;
  // Serial1 wird bereits in setup() initialisiert (31250 Baud)
}

/**
 * Update-Funktion - rufe in loop() auf
 * Liest MIDI Input von Serial1 und prüft Timeout
 */
void updateMidiClockReceiver() {
  // Poll Serial1 für MIDI Messages
  while (Serial1.available()) {
    uint8_t midiByte = Serial1.read();
    
    // System Realtime Messages (0xF8-0xFF) haben höchste Priorität
    // und können jederzeit auftreten (auch mitten in anderen Messages)
    switch (midiByte) {
      case MIDI_CLOCK_MSG:
        processMidiClock();
        break;
      case MIDI_START_MSG:
        processMidiStart();
        break;
      case MIDI_CONTINUE_MSG:
        processMidiContinue();
        break;
      case MIDI_STOP_MSG:
        processMidiStop();
        break;
      // Alle anderen Messages ignorieren (für Clock irrelevant)
    }
  }
  
  // Timeout Detection
  if (midiClockActive) {
    if ((micros() - lastMidiClockMicros) > MIDI_CLOCK_TIMEOUT_MICROS) {
      midiClockActive = false;
    }
  }
}

/**
 * Getter für MIDI Clock Status (für andere Layer)
 */
inline bool isMidiClockActive() {
  return midiClockActive;
}

/**
 * Getter für berechnetes BPM (für andere Layer)
 */
inline uint16_t getMidiClockBPM() {
  return calculatedBPM;
}

#endif
