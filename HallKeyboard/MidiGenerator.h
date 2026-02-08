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
bool activeMidiNotes[128];

// ============================================
// EXTERN VARIABLES (Synchronisiert mit HallKeyboard.ino / Controller)
// ============================================
extern int currentOctave;
extern int scaleType;
extern int chordModeType;
extern bool chordModeActive;
extern bool arpeggiatorActive;
extern bool holdMode;
extern bool additiveMode;
extern int heldNote;
extern int heldSwitchIdx;
extern bool heldNotes[NUM_SWITCHES];
extern bool holdModeMidiNotes[128];
extern bool chordNotesActive[NUM_SWITCHES];
extern const int maxChordNotes;
extern const int midiNotes[13];

// Callback Funktionen für die Modi (in HallKeyboard.ino oder den Mode-Dateien)
extern int getChordNote(int switchIndex, int variationType, int noteIndex);
extern void removeNoteFromArpeggiatorMode(int note);
extern void addNoteToArpeggiatorMode(int note);

// ============================================
// MIDI GENERATOR FUNCTIONS
// ============================================

void initMidiGenerator() {
  for (int i = 0; i < 128; i++) {
    activeMidiNotes[i] = false;
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
    activeMidiNotes[i] = false;
  }
}

/**
 * Sende MIDI Note On oder Note Off
 * Inklusive State-Management für LEDs
 */
void sendMidiNote(int cmd, int pitch, int velocity) {
  if (pitch < 0 || pitch >= 128) return;
  
  if (velocity > 0) activeMidiNotes[pitch] = true;
  else activeMidiNotes[pitch] = false;
  
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
      int baseNote = midiNotes[i] + (currentOctave * 12);
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
      int oldBaseNote = midiNotes[heldSwitchIdx] + (currentOctave * 12);
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
          if (holdModeMidiNotes[noteToPlay]) {
            holdModeMidiNotes[noteToPlay] = false;
            sendMidiNote(0x90, noteToPlay, 0x00);
          } else {
            holdModeMidiNotes[noteToPlay] = true;
            sendMidiNote(0x90, noteToPlay, 0x45);
          }
        } else {
          if (heldNote == noteToPlay) {
            holdModeMidiNotes[noteToPlay] = false;
            sendMidiNote(0x90, noteToPlay, 0x00);
            heldNote = -1;
          } else {
            if (heldNote != -1) {
              holdModeMidiNotes[heldNote] = false;
              sendMidiNote(0x90, heldNote, 0x00);
            }
            holdModeMidiNotes[noteToPlay] = true;
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
      int baseNote = midiNotes[i] + (currentOctave * 12);
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
    if (activeMidiNotes[note]) {
      sendMidiNote(0x90, note, 0x00);  // Note Off via Velocity 0
    }
  }
}

/**
 * Reset MIDI Tracking
 */
void resetMidiGenerator() {
  stopAllMidiNotes();
  for (int i = 0; i < 128; i++) {
    activeMidiNotes[i] = false;
  }
}

#endif
