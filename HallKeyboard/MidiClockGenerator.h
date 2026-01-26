/**
 * MIDI CLOCK GENERATOR LAYER
 * 
 * Generiert MIDI Clock Output (24 PPQN):
 * - Synchronisiert mit ArduinoTapTempo BPM
 * - Sendet MIDI Clock Message (0xF8) via Serial1
 * 
 * INPUT:
 *   - tapTempo.getBPM()
 * 
 * OUTPUT:
 *   - MIDI Clock Messages (0xF8) via Serial1
 */

#ifndef MIDI_CLOCK_GENERATOR_H
#define MIDI_CLOCK_GENERATOR_H

#include "ArduinoTapTempo.h"

// ============================================
// MIDI CLOCK STATE
// ============================================

unsigned long lastClockMicros = 0;
unsigned long clockIntervalMicros = 20833; // Default: 120 BPM (20.833 µs pro Pulse)
uint16_t ppqnCounter = 0;
uint16_t masterPulseCounter = 0; // 0-95 (für 4 Beats Synchronisation)
bool midiClockRunning = false;

// Konfiguration für Arpeggiator-Synchronisation
bool stopClockOnArpDeactivate = true; // Ob MIDI STOP gesendet werden soll, wenn ARP stoppt

// ============================================
// EXTERN VARIABLES
// ============================================
extern ArduinoTapTempo tapTempo;

// Extern from MidiClockReceiver
extern bool midiClockActive;
extern uint16_t calculatedBPM;

// ============================================
// MIDI CLOCK CONSTANTS
// ============================================
#define MIDI_CLOCK          0xF8
#define MIDI_START          0xFA
#define MIDI_CONTINUE       0xFB
#define MIDI_STOP           0xFC
#define PPQN_VALUE          24

// ============================================
// MIDI CLOCK FUNCTIONS
// ============================================

/**
 * Initialisiert den Clock Generator
 */
void initMidiClockGenerator() {
  lastClockMicros = micros();
  ppqnCounter = 0;
  midiClockRunning = false;
}

/**
 * Berechne Clock-Intervall basierend auf aktuellem BPM
 * Formel: Mikrosekunden pro Viertelnote / 24 Pulses
 * Priorisiert MIDI Clock Input über TapTempo
 */
void updateClockInterval() {
  float bpm;
  
  // Wähle BPM Quelle: MIDI Clock hat Vorrang
  if (midiClockActive) {
    bpm = calculatedBPM; // Von MIDI Clock Receiver
    // Serial.print("MIDI Clock BPM: ");
  } else {
    bpm = tapTempo.getBPM(); // Von internem TapTempo
    // Serial.print("TapTempo BPM: ");
  }
  
  if (bpm <= 0) bpm = 120.0; // Sicherheitsnetz
  clockIntervalMicros = (60000000.0 / bpm) / PPQN_VALUE;
  
  // Serial.println(bpm);
}

/**
 * Setzt die Phase der internen Clock zurück.
 * Nützlich bei Empfang von externem MIDI Start.
 */
void syncMidiClockPhase() {
  ppqnCounter = 0;
  masterPulseCounter = 0;
  lastClockMicros = micros();
}

/**
 * Sende MIDI Clock Start Message
 */
void startMidiClock() {
  Serial1.write(MIDI_START);
  syncMidiClockPhase();
  midiClockRunning = true;
}

/**
 * Sende MIDI Clock Stop Message
 */
void stopMidiClock() {
  Serial1.write(MIDI_STOP);
  midiClockRunning = false;
}

/**
 * Sende MIDI Clock Continue Message
 */
void continueMidiClock() {
  Serial1.write(MIDI_CONTINUE);
  lastClockMicros = micros();
  midiClockRunning = true;
}

/**
 * Verarbeitet einen externen MIDI Clock Pulse.
 * Synchronisiert die interne Phase mit der externen Quelle,
 * um Drift zu vermeiden.
 */
void handleExternalClockPulse() {
  ppqnCounter = (ppqnCounter + 1) % PPQN_VALUE;
  masterPulseCounter = (masterPulseCounter + 1) % 96;
  
  // Setze den internen Timer zurück, damit der hausinterne Generator
  // nicht im Konflikt mit dem externen Takt eigene Pulse sendet.
  lastClockMicros = micros();
}

/**
 * Synchronisiere Clock bei BPM-Änderung
 * Rufe auf wenn tapTempo.update() einen neuen Tap erkennt
 */
void syncMidiClockToBPM() {
  updateClockInterval();
  // Phase zurücksetzen bei manuellem Tap (Downbeat Sync)
  ppqnCounter = 0;
  masterPulseCounter = 0;
  lastClockMicros = micros();
  // Optional: MIDI Start senden um Downbeat zu markieren
  Serial1.write(MIDI_START);
}

/**
 * Haupt-Update-Funktion - rufe in loop() auf
 * Sendet MIDI Clock Pulse (0xF8) alle clockIntervalMicros
 */
void updateMidiClockGenerator() {
  // Wenn externe Clock aktiv ist, pausieren wir den internen Timer-basierten Generator,
  // da wir stattdessen über handleExternalClockPulse() synchronisiert werden.
  if (!midiClockRunning || midiClockActive) return; 
  
  unsigned long currentMicros = micros();
  
  // Prüfe ob genug Zeit vergangen ist für nächsten Pulse
  // Berücksichtigt Überlauf von micros() automatisch durch unsigned subtraction
  if (currentMicros - lastClockMicros >= clockIntervalMicros) {
    
    // Sende MIDI Clock Pulse
    Serial1.write(MIDI_CLOCK);
    
    // Update Counter
    ppqnCounter = (ppqnCounter + 1) % PPQN_VALUE;
    masterPulseCounter = (masterPulseCounter + 1) % 96; // 4 Beats a 24 PPQN
    
    // Setze nächsten Zeitpunkt (addiere Intervall statt currentMicros zu setzen, 
    // um Drift durch Prozesszeit zu vermeiden)
    lastClockMicros += clockIntervalMicros;

    // Optionaler Debug Output (jeden Beat)
    if (ppqnCounter == 0) {
      // Serial.print("MIDI Clock Beat | PPQN: ");
      // Serial.print(ppqnCounter);
      // Serial.print(" | BPM: ");
      // Serial.println(tapTempo.getBPM());
    }
  }
}

#endif
