/**
 * HARDWARE CONTROLLER LAYER
 * 
 * Isolierte Logik für Hardware-Input:
 * - Liest Hall Effect Sensors (Note Switches)
 * - Liest Function Switches (FS1-FS4)
 * - Gibt Events weiter an andere Layer
 * 
 * INPUT: Raw Sensor Values von Arduino Pins
 * OUTPUT: 
 *   - switch_triggered[] - welche Switches gerade gedrückt wurden (einmaliger Trigger)
 *   - switch_released[] - welche Switches gerade losgelassen wurden
 *   - switch_held[] - welche Switches gerade gehalten sind
 */

#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include "button.h"
#include "arduino_stubs.h"

// ============================================
// HARDWARE CONSTANTS
// ============================================

const int NUM_SWITCHES = 13;

// Pin-Belegung für alle 13 Switches
const int switchPins[NUM_SWITCHES] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  18 // D18 (A4) als Digital
};

// MIDI Noten für jeden Switch (C bis C, relativ zu Oktave 0)
const int midiNotes[NUM_SWITCHES] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

// LED-Mapping: Switch-Index -> LED-Index
const int ledMapping[NUM_SWITCHES] = {
  0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6, 7
};

// Array zum Identifizieren von schwarzen Tasten
const bool isBlackKey[NUM_SWITCHES] = {
  false, true, false, true, false, false, true, false, true, false, true, false, false
};

// Funktions-Schalter (4 Schalter an A1-A4)
const int NUM_FUNCTION_SWITCHES = 4;
const int functionSwitchPins[NUM_FUNCTION_SWITCHES] = {
  A1, A2, A3, A4
};

const unsigned long LONG_PRESS_DURATION = 1000;

// ============================================
// HARDWARE CONTROLLER STATE
// ============================================

// Button-Objekte
extern Button switches[NUM_SWITCHES];
extern Button functionSwitches[NUM_FUNCTION_SWITCHES];

// Events für diesen Frame
extern bool switch_triggered[NUM_SWITCHES];
extern bool switch_released[NUM_SWITCHES];
extern bool switch_held[NUM_SWITCHES];

// Function Switch State
extern unsigned long functionSwitchPressTime[NUM_FUNCTION_SWITCHES];
extern bool functionSwitchLongPressed[NUM_FUNCTION_SWITCHES];

// Externals defined elsewhere
extern int currentOctave;

// ============================================
// HARDWARE CONTROLLER: Buttons & Pins (Actual Instance)
// ============================================

// Wir definieren die Instanzen hier, falls sie nirgendwo sonst definiert sind.
// Da wir sie aus der .ino entfernen, werden sie hier angelegt.
Button switches[NUM_SWITCHES];
Button functionSwitches[NUM_FUNCTION_SWITCHES];
bool switch_triggered[NUM_SWITCHES];
bool switch_released[NUM_SWITCHES];
bool switch_held[NUM_SWITCHES];
unsigned long functionSwitchPressTime[NUM_FUNCTION_SWITCHES];
bool functionSwitchLongPressed[NUM_FUNCTION_SWITCHES];

// ============================================
// HARDWARE CONTROLLER UPDATE & SETUP
// ============================================

/**
 * Initialisiere alle Pins und Buttons
 */
void setupHardwareController() {
  // Alle Switch-Pins initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switches[i].begin(switchPins[i]);
    switch_triggered[i] = false;
    switch_released[i] = false;
    switch_held[i] = false;
  }
  
  // Analog Pins als Digital mit Pullup konfigurieren
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  
  // Funktions-Schalter initialisieren
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    functionSwitches[i].begin(functionSwitchPins[i]);
    functionSwitchLongPressed[i] = false;
    functionSwitchPressTime[i] = 0;
  }
}

/**
 * Aktualisiere Hardware-Input für diesen Frame
 */
void updateHardwareController() {
  // Setze alle Events für diesen Frame zurück
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switch_triggered[i] = false;
    switch_released[i] = false;
  }
  
  // Aktualisiere alle Note-Switches
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (switches[i].trigger()) {
      switch_triggered[i] = true;
      switch_held[i] = true;
    }
    else if (switches[i].released()) {
      switch_released[i] = true;
      switch_held[i] = false;
    }
  }
}

/**
 * Hilfsfunktion: Gib MIDI-Note für einen Switch-Index
 */
int getHardwareMIDINote(int switchIndex) {
  if (switchIndex < 0 || switchIndex >= NUM_SWITCHES) {
    return -1;
  }
  return midiNotes[switchIndex] + (currentOctave * 12);
}

#endif
