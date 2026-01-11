/**
 * ARPEGGIATOR MODE LAYER
 * 
 * Isolierte Logik für Arpeggiator:
 * - Verwaltet gehaltene Noten
 * - Sequenziert Noten basierend auf Modus (Up, Down, Up-Down, Down-Up)
 * - Rhythmische Kontrolle basierend auf Tap Tempo
 * - 50% Duty Cycle für Note ON/OFF
 * 
 * INPUT:
 *   - switch_triggered[], switch_released[] vom Hardware Controller
 *   - arpeggiatorActive, arpeggiatorMode, arpeggiatorRate aus Software Controller
 *   - tapTempo.getBeatLength()
 * 
 * OUTPUT:
 *   - arpeggiatorMidiNotes[] - Welche Note soll gerade vom Arp spielen
 */

#ifndef ARPEGGIATOR_MODE_H
#define ARPEGGIATOR_MODE_H

#include "ArduinoTapTempo.h"

// ============================================
// ARPEGGIATOR MODE STATE & CONFIG
// ============================================

bool arpeggiatorMidiNotes[128];

int arpeggiatorMode = 0;       // Default: ARPEGGIATOR_UP_DOWN (0)
int arpeggiatorRate = 2;       // Default: RATE_EIGHTH
unsigned long lastArpeggiatorStepTime = 0;
unsigned long arpeggiatorStepDuration = 250;
unsigned long arpeggiatorNoteOnTime = 0;
int arpeggiatorDutyCycle = 50;
int currentArpeggiatorPlayingNote = -1;
bool arpeggiatorNoteIsOn = false;
float lastArpeggiatorSyncProgress = 0;
int arpeggiatorBeatCounter = 0;
float lastArpeggiatorRawProgress = 0;

int heldArpeggiatorNotes[13];
int numHeldArpeggiatorNotes = 0;
uint8_t arpNoteRefCount[128];
int currentArpeggiatorIndex = 0;
bool arpeggiatorAscending = true;

// Tap Tempo Instanz
ArduinoTapTempo tapTempo;

// ============================================
// EXTERNE VARIABLEN & FUNKTIONEN
// ============================================
extern bool arpeggiatorActive;
extern void sendMidiNote(int cmd, int pitch, int velocity);

// Konstanten
#ifndef ARPEGGIATOR_UP_DOWN
#define ARPEGGIATOR_UP_DOWN 0
#define ARPEGGIATOR_DOWN_UP 1
#define ARPEGGIATOR_UP 2
#define ARPEGGIATOR_DOWN 3
#define ARPEGGIATOR_SEQUENCE 4
#endif

// ============================================
// FORWARD DECLARATIONS
// ============================================
void playNextArpeggiatorNote();

// ============================================
// ARPEGGIATOR MODE FUNCTIONS
// ============================================

void initArpeggiatorMode() {
  for (int i = 0; i < 128; i++) {
    arpeggiatorMidiNotes[i] = false;
  }
  for (int i = 0; i < 13; i++) {
    heldArpeggiatorNotes[i] = -1;
  }
  for (int i = 0; i < 128; i++) {
    arpNoteRefCount[i] = 0;
  }
  numHeldArpeggiatorNotes = 0;
}

/**
 * Aktualisiere Arpeggiator in jedem Loop-Durchgang
 * - Berechne neue Step-Dauer basierend auf Tap Tempo
 * - Prüfe ob Note OFF werden soll (Duty Cycle)
 * - Prüfe ob nächste Note spielen soll
 */
void updateArpeggiatorMode() {
  // Wenn nicht aktiv, nichts tun
  if (!arpeggiatorActive) {
    return;
  }
  
  // Berechne StepDuration basierend auf Rate und Tap Tempo
  unsigned long beatLength = tapTempo.getBeatLength();
  if (beatLength <= 0) return;

  float divisions = 1.0;
  if (arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
    // Im Sequence Mode ist die Rate fest auf Whole Note (4 Beats)
    divisions = 0.25;
  } else {
    switch(arpeggiatorRate) {
      case RATE_WHOLE:        divisions = 0.25; break;
      case RATE_QUARTER:      divisions = 1.0; break;
      case RATE_EIGHTH:       divisions = 2.0; break;
      case RATE_SIXTEENTH:    divisions = 4.0; break;
      case RATE_TRIPLET:      divisions = 3.0; break;
      default:                divisions = 2.0; break;
    }
  }
  
  arpeggiatorStepDuration = beatLength / divisions;
  
  unsigned long currentTime = millis();
  float currentRawProgress = tapTempo.beatProgress();
  
  // Update beat counter based on wrap-around of raw progress
  if (currentRawProgress < lastArpeggiatorRawProgress) {
    arpeggiatorBeatCounter = (arpeggiatorBeatCounter + 1) % 4; // Cycle through 4 beats
  }
  lastArpeggiatorRawProgress = currentRawProgress;

  // Calculate continuous progress across 4 beats (0.0 to 4.0)
  float continuousProgress = (float)arpeggiatorBeatCounter + currentRawProgress;
  
  // Scaled progress determines the trigger points
  float scaledProgress = continuousProgress * divisions;
  
  // Trigger Logic: Prüfe ob Master-Phase eine Schwelle überschritten hat (Phasen-Lock zum Tap Tempo)
  bool trigger = false;
  if (floor(scaledProgress) != floor(lastArpeggiatorSyncProgress)) {
    trigger = true;
  } 
  // Spezialfall Wrap-around der skalierten Phase (z.B. alle 4 Beats bei Whole Note)
  else if (scaledProgress < lastArpeggiatorSyncProgress) {
    trigger = true;
  }

  // Nur spielen, wenn auch Noten da sind
  if (trigger && numHeldArpeggiatorNotes > 0) {
    playNextArpeggiatorNote();
    arpeggiatorNoteOnTime = currentTime;
  }

  // Immer den Sync-Status aktualisieren
  lastArpeggiatorSyncProgress = scaledProgress;

  // Duty Cycle: Prüfe ob Note OFF sein sollte
  // Im Sequence Mode fest 99% für Legato-Effekt, sonst nutzen wir den globalen Wert
  if (numHeldArpeggiatorNotes > 0) {
    int dutyCycle = (arpeggiatorMode == ARPEGGIATOR_SEQUENCE) ? 99 : arpeggiatorDutyCycle;
    if (arpeggiatorNoteIsOn && currentTime - arpeggiatorNoteOnTime >= (arpeggiatorStepDuration * dutyCycle / 100)) {
      // Zeit für Note Off!
      if (currentArpeggiatorPlayingNote >= 0 && currentArpeggiatorPlayingNote < 128) {
        sendMidiNote(0x80, currentArpeggiatorPlayingNote, 0);
        arpeggiatorNoteIsOn = false;
      }
    }
  }
}

/**
 * Berechne und spiele die nächste Note basierend auf Modus
 */
void playNextArpeggiatorNote() {
  // Sicherheitschecks
  if (numHeldArpeggiatorNotes == 0) {
    arpeggiatorActive = false;
    return;
  }
  
  // Lokale Kopie für Sortierung und Zugriff
  int activeNotes[13];
  int count = numHeldArpeggiatorNotes;
  for (int i = 0; i < count; i++) {
    activeNotes[i] = heldArpeggiatorNotes[i];
  }

  // Sortiere Noten nach Tonhöhe (außer im SEQUENCE Modus)
  if (arpeggiatorMode != ARPEGGIATOR_SEQUENCE) {
    for (int i = 0; i < count - 1; i++) {
      for (int j = 0; j < count - i - 1; j++) {
        if (activeNotes[j] > activeNotes[j+1]) {
          int temp = activeNotes[j];
          activeNotes[j] = activeNotes[j+1];
          activeNotes[j+1] = temp;
        }
      }
    }
  }
  
  int nextIndex = 0;
  
  if (currentArpeggiatorIndex == -1) {
    // Erste Note nach Ruhepause: Bestimme Start-Index und Richtung basierend auf Modus
    if (arpeggiatorMode == ARPEGGIATOR_DOWN || arpeggiatorMode == ARPEGGIATOR_DOWN_UP) {
      nextIndex = count - 1;
      arpeggiatorAscending = false;
    } else {
      nextIndex = 0;
      arpeggiatorAscending = true;
    }
  } else if (count == 1) {
    // Single Note Special: Bleibe immer bei Index 0 und pulse die Note
    nextIndex = 0;
  } else {
    // Mehrere Noten - berechne nächsten Index basierend auf Modus
    switch(arpeggiatorMode) {
      case ARPEGGIATOR_UP: {
        nextIndex = (currentArpeggiatorIndex + 1) % count;
        break;
      }
      
      case ARPEGGIATOR_DOWN: {
        nextIndex = (currentArpeggiatorIndex - 1 + count) % count;
        break;
      }
      
      case ARPEGGIATOR_UP_DOWN: {
        if (arpeggiatorAscending) {
          nextIndex = currentArpeggiatorIndex + 1;
          if (nextIndex >= count) {
            nextIndex = count - 2;
            arpeggiatorAscending = false;
          }
        } else {
          nextIndex = currentArpeggiatorIndex - 1;
          if (nextIndex < 0) {
            nextIndex = 1;
            arpeggiatorAscending = true;
          }
        }
        break;
      }
      
      case ARPEGGIATOR_DOWN_UP: {
        if (arpeggiatorAscending) {
          nextIndex = currentArpeggiatorIndex + 1;
          if (nextIndex >= count) {
            nextIndex = count - 2;
            arpeggiatorAscending = false;
          }
        } else {
          nextIndex = currentArpeggiatorIndex - 1;
          if (nextIndex < 0) {
            nextIndex = 1;
            arpeggiatorAscending = true;
          }
        }
        break;
      }

      case ARPEGGIATOR_SEQUENCE: {
        nextIndex = (currentArpeggiatorIndex + 1) % count;
        break;
      }
    }
  }
  
  // Sicherheitshalber Grenzen prüfen
  if (nextIndex < 0) nextIndex = 0;
  if (nextIndex >= count) nextIndex = 0;

  // Spiele Note am nextIndex aus unserer (ggf. sortierten) Liste
  int noteToPlay = activeNotes[nextIndex];
  
  if (noteToPlay >= 0 && noteToPlay < 128) {
    // Schalte alte Note aus
    if (currentArpeggiatorPlayingNote >= 0 && currentArpeggiatorPlayingNote < 128) {
      sendMidiNote(0x80, currentArpeggiatorPlayingNote, 0);
    }
    
    // Spiele neue Note
    sendMidiNote(0x90, noteToPlay, 0x45);
    currentArpeggiatorPlayingNote = noteToPlay;
    arpeggiatorNoteIsOn = true;
    arpeggiatorNoteOnTime = millis();
  }
  
  currentArpeggiatorIndex = nextIndex;
}

/**
 * Füge eine Note zur Arpeggiator-Sequenz hinzu
 */
void addNoteToArpeggiatorMode(int note) {
  if (note < 0 || note >= 128) return;
  
  // Increment Reference Count
  arpNoteRefCount[note]++;
  
  // Check if note is already in the list
  if (arpNoteRefCount[note] > 1) {
    return; // Already present
  }
  
  if (numHeldArpeggiatorNotes >= 13) return;
  
  // Wenn das die erste Note ist, setzen wir den Index so zurück,
  // dass beim nächsten Beat-Trigger die erste Note (Index 0) spielt.
  if (numHeldArpeggiatorNotes == 0) {
    currentArpeggiatorIndex = -1; // Spezialwert für Start
  }

  // Füge Note hinzu
  heldArpeggiatorNotes[numHeldArpeggiatorNotes] = note;
  numHeldArpeggiatorNotes++;
}

/**
 * Transponiert alle aktuell im Arpeggiator gespeicherten Noten
 */
void transposeArpeggiatorNotes(int semiTones) {
  for (int i = 0; i < numHeldArpeggiatorNotes; i++) {
    int newNote = heldArpeggiatorNotes[i] + semiTones;
    if (newNote >= 0 && newNote < 128) {
      heldArpeggiatorNotes[i] = newNote;
    }
  }
}

/**
 * Entferne eine Note aus der Arpeggiator-Sequenz
 */
void removeNoteFromArpeggiatorMode(int note) {
  if (note < 0 || note >= 128) return;
  
  if (arpNoteRefCount[note] > 0) {
    arpNoteRefCount[note]--;
  }
  
  // Nur entfernen wenn RefCount 0
  if (arpNoteRefCount[note] > 0) {
    return;
  }
  
  for (int i = 0; i < numHeldArpeggiatorNotes; i++) {
    if (heldArpeggiatorNotes[i] == note) {
      // Verschiebe alle Noten nach vorne
      for (int j = i; j < numHeldArpeggiatorNotes - 1; j++) {
        heldArpeggiatorNotes[j] = heldArpeggiatorNotes[j + 1];
      }
      numHeldArpeggiatorNotes--;
      
      // Wenn keine Noten mehr, schalte aktuelle Note aus
      if (numHeldArpeggiatorNotes == 0 && currentArpeggiatorPlayingNote >= 0 && currentArpeggiatorPlayingNote < 128) {
        sendMidiNote(0x80, currentArpeggiatorPlayingNote, 0);
        currentArpeggiatorPlayingNote = -1;
      }
      
      // Setze Index zurück wenn nötig
      if (currentArpeggiatorIndex >= numHeldArpeggiatorNotes && numHeldArpeggiatorNotes > 0) {
        currentArpeggiatorIndex = numHeldArpeggiatorNotes - 1;
      }
      return;
    }
  }
}

/**
 * Gib Array von Noten die der Arpeggiator spielt
 */
// bool* getArpeggiatorModeNotes() {
//   return arpeggiatorMidiNotes;
// }

/**
 * Clear all arpeggiator notes
 */
void clearArpeggiatorNotes() {
  for (int i = 0; i < 128; i++) {
    arpeggiatorMidiNotes[i] = false;
    arpNoteRefCount[i] = 0;
  }
  numHeldArpeggiatorNotes = 0;
  currentArpeggiatorPlayingNote = -1;
  arpeggiatorNoteIsOn = false;
}

#endif
