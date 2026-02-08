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

volatile unsigned long lastClockMicros = 0;
unsigned long clockIntervalMicros = 20833; // Default: 120 BPM
volatile uint16_t ppqnCounter = 0;
volatile uint16_t masterPulseCounter = 0; // 0-95 (für 4 Beats Synchronisation)
volatile bool midiClockRunning = false;

// Konfiguration für Arpeggiator-Synchronisation
bool stopClockOnArpDeactivate = true; // Ob MIDI STOP gesendet werden soll, wenn ARP stoppt

// ============================================
// EXTERN VARIABLES
// ============================================
extern ArduinoTapTempo tapTempo;

// Extern from MidiClockReceiver
extern volatile bool midiClockActive;
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
 * ISR für Timer 1 (MIDI Clock Pulse)
 * Hier wird der eigentliche MIDI Pulse gesendet.
 * Durch die ISR hat die Clock Vorrang vor der LED-Logik.
 */
ISR(TIMER1_COMPA_vect) {
  if (midiClockRunning && !midiClockActive) {
    Serial1.write(MIDI_CLOCK);
    ppqnCounter = (ppqnCounter + 1) % PPQN_VALUE;
    masterPulseCounter = (masterPulseCounter + 1) % 96; // 4 Beats a 24 PPQN
    lastClockMicros = micros();
  }
}

/**
 * Initialisiert den Clock Generator (Timer 1 Setup)
 */
void initMidiClockGenerator() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // Startwert: 120 BPM
  // 16MHz / 64 prescaler = 250kHz
  // OCR1A = 625000 / 120 = 5208
  OCR1A = 5208;
  
  TCCR1B |= (1 << WGM12);               // CTC Mode
  TCCR1B |= (1 << CS11) | (1 << CS10);  // Prescaler 64
  TIMSK1 |= (1 << OCIE1A);              // Enable Compare A Interrupt
  sei();

  lastClockMicros = micros();
  ppqnCounter = 0;
  midiClockRunning = false;
}

/**
 * Berechne Clock-Intervall basierend auf aktuellem BPM
 * Passt den Timer-Compare-Wert (OCR1A) an.
 */
void updateClockInterval() {
  float bpm;
  
  if (midiClockActive) {
    bpm = calculatedBPM;
  } else {
    bpm = tapTempo.getBPM();
  }
  
  if (bpm <= 0.0) bpm = 120.0;
  
  // Timer-Ticks berechnen (16MHz / 64 = 250kHz)
  // Ticks = 250000 / (bpm * 24 / 60) = 625000 / bpm
  uint16_t ticks = (uint16_t)(625000.0 / bpm);
  
  cli();
  OCR1A = ticks;
  sei();
  
  clockIntervalMicros = (60000000.0 / bpm) / PPQN_VALUE;
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
  TCNT1 = 0; 
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
  TCNT1 = 0;
  lastClockMicros = micros();
  // Optional: MIDI Start senden um Downbeat zu markieren
  Serial1.write(MIDI_START);
}

#endif
