/**
 * MIDI GENERATOR LAYER
 * 
 * Isolierte Logik für MIDI-Ausgabe:
 * - Nimmt Noten-Arrays von Hold Mode, Chord Mode, Arpeggiator
 * - Generiert MIDI Note On/Off Signale
 * - Verwaltet activeMidiNotes[] für LED-Anzeige
 * 
 * INPUT:
 *   - holdModeMidiNotes[] vom Hold Mode Layer
 *   - chordModeMidiNotes[] vom Chord Mode Layer
 *   - arpeggiatorMidiNotes[] vom Arpeggiator Mode Layer
 * 
 * OUTPUT:
 *   - MIDI Signale via Serial1
 *   - activeMidiNotes[] für LED Controller
 */

#ifndef MIDI_GENERATOR_H
#define MIDI_GENERATOR_H

#include "HardwareController.h"

// ============================================
// MIDI GENERATOR STATE
// ============================================

// Tracke alle aktiven MIDI-Noten (von allen Modi kombiniert)
// Eingespart: Bitset statt bool[128]
uint8_t activeMidiNotes[16];

#define IS_NOTE_ACTIVE(n) ((activeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_NOTE_ACTIVE(n, v) if(v) activeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else activeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))

#define IS_HOLD_NOTE_ACTIVE(n) ((holdModeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_HOLD_NOTE_ACTIVE(n, v) if(v) holdModeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else holdModeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))

// ============================================
// EXTERN VARIABLES (Synchronisiert mit HallKeyboard.ino / Controller)
// ============================================
extern int8_t currentOctave;
extern int8_t scaleType;
extern int8_t chordModeType;
extern bool chordModeActive;
extern bool arpeggiatorActive;
extern bool holdMode;
extern bool additiveMode;
extern int8_t heldNote;
extern int8_t heldSwitchIdx;
extern bool heldNotes[NUM_SWITCHES];
extern uint8_t holdModeMidiNotes[16];
extern bool chordNotesActive[NUM_SWITCHES];
extern const uint8_t maxChordNotes;
extern const uint8_t midiNotes[13];

// Callback Funktionen für die Modi (in HallKeyboard.ino oder den Mode-Dateien)
extern int getChordNote(int switchIndex, int variationType, int noteIndex);
extern void removeNoteFromArpeggiatorMode(int note);
extern void addNoteToArpeggiatorMode(int note);

// ============================================
// MIDI GENERATOR FUNCTIONS
// ============================================

void initMidiGenerator() {
  for (int i = 0; i < 16; i++) {
    activeMidiNotes[i] = 0;
  }
}

/**
 * Sendet MIDI "All Notes Off" und schaltet alle aktiven Noten im Speicher aus.
 * Nützlich beim Bootup oder bei "Panic" Situationen.
 */
void killAllMidiNotes() {
  // Option 1: MIDI Control Change 123 (All Notes Off) auf Kanal 1
  Serial1.write(0xB0); 
  Serial1.write(123); 
  Serial1.write(0);
  
  // Option 2: Explizite Note Offs für alle 128 Noten (Sicherheitsnetz)
  for (int i = 0; i < 128; i++) {
    Serial1.write(0x80); 
    Serial1.write(i);
    Serial1.write(0);
  }
  for (int i = 0; i < 16; i++) activeMidiNotes[i] = 0;
}

/**
 * Sende MIDI Note On oder Note Off
 * Inklusive State-Management für LEDs
 */
void sendMidiNote(int cmd, int pitch, int velocity) {
  if (pitch < 0 || pitch >= 128) return;
  
  SET_NOTE_ACTIVE(pitch, (velocity > 0));
  
  Serial1.write(cmd);
  Serial1.write(pitch);
  Serial1.write(velocity);
}

/**
 * Aktualisiere alle MIDI-Noten basierend auf allen aktiven Modi
 * Diese Funktion wird jede Loop aufgerufen und koordiniert
 * welche Noten gespielt werden sollen (State Sync)
 */
void updateMidiGenerator() {
  // Aktuell primär eventbasiert, State Sync Logik hier möglich
}

/**
 * Stoppe alle MIDI-Noten
 */
void stopAllMidiNotes() {
  for (int note = 0; note < 128; note++) {
    if (IS_NOTE_ACTIVE(note)) {
      sendMidiNote(0x90, note, 0x00);  // Note Off via Velocity 0
    }
  }
}

/**
 * Reset MIDI Tracking
 */
void resetMidiGenerator() {
  stopAllMidiNotes();
  initMidiGenerator();
}

#endif
