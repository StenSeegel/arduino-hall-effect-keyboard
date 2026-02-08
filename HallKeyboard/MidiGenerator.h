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
 * Kern-Logik für Noten-Transformation (Scale, Chord, Arp)
 * Diese Funktion verarbeitet einen einzelnen Switch-Event
 * und delegiert an die entsprechenden Modi.
 */
void handleMidiNoteEvent(int i, bool isTriggered, bool isReleased) {
  int currentNote = getHardwareMIDINote(i);
  
  if (isTriggered) {
    int notesToPlay[5];
    int numNotesToPlay = 1;
    
    // 1. Akkord-Berechnung
    if (chordModeActive && chordModeType != 0) { // CHORD_MODE_OFF
      bool isFolded = (chordModeType == 2); // CHORD_MODE_FOLDED
      int baseNote = pgm_read_byte(&midiNotes[i]) + (currentOctave * 12);
      numNotesToPlay = 0;
      for (int j = 0; j < maxChordNotes; j++) {
        int noteOffset = getChordNote(i, scaleType, j);
        if (noteOffset >= 0 && numNotesToPlay < 5) {
          int chordNote = baseNote + noteOffset;
          // Folding logic
          if (isFolded) {
            while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
            while (chordNote < currentOctave * 12) chordNote += 12;
          }
          notesToPlay[numNotesToPlay] = chordNote;
          numNotesToPlay++;
        }
      }
    } else {
      notesToPlay[0] = currentNote;
      numNotesToPlay = 1;
    }
    
    // 2. HOLD + ARP Special: Vorherige Note entfernen wenn Hold aktiv (Nur im Mono-Hold)
    if (holdMode && arpeggiatorActive && !additiveMode && heldSwitchIdx != -1 && heldSwitchIdx != i) {
      int oldBaseNote = pgm_read_byte(&midiNotes[heldSwitchIdx]) + (currentOctave * 12);
      if (chordModeActive && chordModeType != 0) {
        bool isFoldedCheck = (chordModeType == 2);
        for (int j = 0; j < maxChordNotes; j++) {
          int noteOffsetCheck = getChordNote(heldSwitchIdx, scaleType, j);
          if (noteOffsetCheck >= 0) {
            int oldChordNote = oldBaseNote + noteOffsetCheck;
            if (isFoldedCheck) {
              while (oldChordNote > (currentOctave + 1) * 12) oldChordNote -= 12;
              while (oldChordNote < currentOctave * 12) oldChordNote += 12;
            }
            removeNoteFromArpeggiatorMode(oldChordNote);
          }
        }
      } else {
        removeNoteFromArpeggiatorMode(oldBaseNote);
      }
    }
    
    // Update Hold State
    bool isTriggeringNew;
    if (additiveMode) {
      heldNotes[i] = !heldNotes[i]; // Toggle
      isTriggeringNew = heldNotes[i];
    } else {
      bool isSameSwitchDouble = (heldSwitchIdx == i);
      if (holdMode && !isSameSwitchDouble) {
        if (heldSwitchIdx != -1) heldNotes[heldSwitchIdx] = false; // Synchronisiere heldNotes
        heldSwitchIdx = i;
        heldNotes[i] = true;
        isTriggeringNew = true;
      } else if (holdMode && isSameSwitchDouble) {
        heldSwitchIdx = -1;
        heldNotes[i] = false;
        isTriggeringNew = false;
      } else {
        isTriggeringNew = true;
      }
    }
    
    // 3. Noten spielen / zum Arp hinzufügen
    for (int noteIdx = 0; noteIdx < numNotesToPlay; noteIdx++) {
      int noteToPlay = notesToPlay[noteIdx];
      
      if (holdMode && arpeggiatorActive) {
        if (isTriggeringNew) addNoteToArpeggiatorMode(noteToPlay);
        else removeNoteFromArpeggiatorMode(noteToPlay);
      } else if (holdMode) {
        if (additiveMode) {
          if (IS_HOLD_NOTE_ACTIVE(noteToPlay)) {
            SET_HOLD_NOTE_ACTIVE(noteToPlay, false);
            sendMidiNote(0x90, noteToPlay, 0x00);
          } else {
            SET_HOLD_NOTE_ACTIVE(noteToPlay, true);
            sendMidiNote(0x90, noteToPlay, 0x45);
          }
        } else {
          if (heldNote == noteToPlay) {
            SET_HOLD_NOTE_ACTIVE(noteToPlay, false);
            sendMidiNote(0x90, noteToPlay, 0x00);
            heldNote = -1;
          } else {
            if (heldNote != -1) {
              SET_HOLD_NOTE_ACTIVE(heldNote, false);
              sendMidiNote(0x90, heldNote, 0x00);
            }
            SET_HOLD_NOTE_ACTIVE(noteToPlay, true);
            sendMidiNote(0x90, noteToPlay, 0x45);
            heldNote = noteToPlay;
          }
        }
      } else if (arpeggiatorActive) {
        addNoteToArpeggiatorMode(noteToPlay);
      } else {
        sendMidiNote(0x90, noteToPlay, 0x45);
      }
    }
  }
  
  if (isReleased) {
    int notesToRelease[5];
    int numNotesToRelease = 1;
    
    if (chordModeActive && chordModeType != 0) {
      bool isFolded = (chordModeType == 2);
      int baseNote = pgm_read_byte(&midiNotes[i]) + (currentOctave * 12);
      numNotesToRelease = 0;
      for (int j = 0; j < maxChordNotes; j++) {
        int noteOffset = getChordNote(i, scaleType, j);
        if (noteOffset >= 0 && numNotesToRelease < 5) {
          int chordNote = baseNote + noteOffset;
          if (isFolded) {
            while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
            while (chordNote < currentOctave * 12) chordNote += 12;
          }
          notesToRelease[numNotesToRelease] = chordNote;
          numNotesToRelease++;
        }
      }
    } else {
      notesToRelease[0] = currentNote;
      numNotesToRelease = 1;
    }
    
    for (int noteIdx = 0; noteIdx < numNotesToRelease; noteIdx++) {
      int noteToRelease = notesToRelease[noteIdx];
      if (!holdMode) {
        if (arpeggiatorActive) removeNoteFromArpeggiatorMode(noteToRelease);
        else sendMidiNote(0x90, noteToRelease, 0x00);
      }
    }
  }
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
