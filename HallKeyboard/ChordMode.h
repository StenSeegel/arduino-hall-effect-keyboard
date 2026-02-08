/**
 * CHORD MODE LAYER
 * 
 * Isolierte Logik für Chord Mode:
 * - Extended Mode: Akkord-Noten unten/oben (ungefaltet)
 * - Folded Mode: Akkord-Noten in Oktave gefaltet
 * 
 * INPUT:
 *   - switch_triggered[] vom Hardware Controller
 *   - Grundton (MIDI Note)
 *   - scaleType, diatonicRootKey aus Software Controller
 *   - chordModeType (Extended/Folded)
 * 
 * OUTPUT:
 *   - chordModeMidiNotes[] - Welche Noten sollen vom Chord Mode gespielt werden
 */

#ifndef CHORD_MODE_H
#define CHORD_MODE_H

#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel pixels;
extern uint8_t activeMidiNotes[16];
extern uint8_t holdModeMidiNotes[16];
extern uint8_t chordModeMidiNotes[16];

// ============================================
// CHORD MODE CONFIGURATION & CONSTANTS
// ============================================

#define CHORD_MODE_OFF 0
#define CHORD_MODE_EXTENDED 1
#define CHORD_MODE_FOLDED 2

// Chord Extension Types
#define CHORD_EXT_TRIAD 0
#define CHORD_EXT_7TH 1
#define CHORD_EXT_7TH_OCTAVE 2

#define NUM_SCALE_TYPES 9
#define SCALE_IONIAN 0      // Major
#define SCALE_DORIAN 1      // Dorian
#define SCALE_PHRYGIAN 2    // Phrygian
#define SCALE_LYDIAN 3      // Lydian
#define SCALE_MIXOLYDIAN 4  // Mixolydian
#define SCALE_AEOLIAN 5     // Natural Minor
#define SCALE_LOCRIAN 6     // Locrian
#define SCALE_POWER5 7      // Power 5
#define SCALE_POWER8 8      // Power 8

#define ROOT_C 0
#define ROOT_CS 1
#define ROOT_D 2
#define ROOT_DS 3
#define ROOT_E 4
#define ROOT_F 5
#define ROOT_FS 6
#define ROOT_G 7
#define ROOT_GS 8
#define ROOT_A 9
#define ROOT_AS 10
#define ROOT_B 11

int8_t chordModeType = 0;              // 0=off, 1=extended, 2=folded
int8_t scaleType = 0;                  // 0-8: Alle diatonischen Modi + Power Chords
int8_t diatonicRootKey = 0;            // 0-11 entspricht C-B
uint8_t chordExtensionType = CHORD_EXT_TRIAD; // 0=Triad, 1=7th, 2=7th+8th

const uint8_t maxChordNotes = 5;

// Akkordtypen: [Index] = {Semitone offsets}
const int8_t chordDefinitions[7][5] PROGMEM = {
  {0, 4, 7, -1, -1},    // 0 = Major
  {0, 3, 7, -1, -1},    // 1 = Minor
  {0, 7, -1, -1, -1},   // 2 = Power 5 (ohne 3. Note)
  {0, 7, 12, -1, -1},   // 3 = Power 8 (mit Oktave)
  {0, 5, 7, -1, -1},    // 4 = Sus4
  {0, 4, 8, -1, -1},    // 5 = Augmented
  {0, 3, 6, -1, -1}     // 6 = Diminished
};

// Diatonische Akkord-Muster für alle 7 Modi
const int8_t diatonicChordPattern[7] PROGMEM = {0, 1, 1, 0, 0, 1, 6};  // Major, minor, minor, Major, Major, minor, Dim

// Intervallo-Pattern für alle diatonischen Modi
const int8_t modeStepIntervals[7] PROGMEM = {2, 2, 1, 2, 2, 2, 1};  // Intervalle für Ionian

// ============================================
// EXTERN VARIABLES (from HallKeyboard.ino / HardwareController.h)
// ============================================
extern int8_t currentOctave;           // Aktuelle Oktave
extern const uint8_t midiNotes[13];     // MIDI Notes für die Tasten
extern void sendMidiNote(int cmd, int pitch, int velocity);

// ============================================
// CHORD MODE STATE
// ============================================

// Tracke welche Noten im Chord Mode gerade spielen
uint8_t chordModeMidiNotes[16];        // Welche MIDI-Noten sind von Chord Mode aktiv

#define IS_NOTE_ACTIVE(n) ((activeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_NOTE_ACTIVE(n, v) if(v) activeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else activeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))
#define IS_CHORD_NOTE_ACTIVE(n) ((chordModeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_CHORD_NOTE_ACTIVE(n, v) if(v) chordModeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else chordModeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))
#define CLEAR_CHORD_NOTES() for(int _i=0; _i<16; _i++) chordModeMidiNotes[_i] = 0

// ============================================
// HELPER FUNCTIONS (Migriert von HallKeyboard.ino)
// ============================================

/**
 * Berechne die Semitone für einen Grad in einem Modus
 */
int getModeNote(int degree, int mode) {
  int semitones = 0;
  for (int i = 0; i < degree; i++) {
    semitones += pgm_read_byte(&modeStepIntervals[(i + mode) % 7]);
  }
  return semitones;
}

/**
 * Prüfe ob eine Note in der diatonischen Tonleiter ist
 */
bool isDiatonicNote(int switchIndex) {
  int noteOffset = (pgm_read_byte(&midiNotes[switchIndex]) - diatonicRootKey + 12) % 12;
  
  if (scaleType >= 0 && scaleType <= 6) {
    for (int i = 0; i < 7; i++) {
      if (noteOffset == getModeNote(i, scaleType)) {
        return true;
      }
    }
    return false;
  }
  
  return true;
}

/**
 * Bestimme den Akkordtyp basierend auf diatonischem Grad
 */
int getDiatonicChordType(int switchIndex) {
  int noteOffset = (pgm_read_byte(&midiNotes[switchIndex]) - diatonicRootKey + 12) % 12;
  
  int diatonicDegree = -1;
  for (int i = 0; i < 7; i++) {
    if (noteOffset == getModeNote(i, scaleType)) {
      diatonicDegree = i;
      break;
    }
  }
  
  if (diatonicDegree == -1) {
    return 0;  // Major als Default
  }
  
  if (scaleType >= 0 && scaleType <= 6) {
    int modalIndex = (diatonicDegree + scaleType) % 7;
    return pgm_read_byte(&diatonicChordPattern[modalIndex]);
  } else {
    return 0;  // Major
  }
}

/**
 * Hole eine diatonische Akkord-Note (Triad/7th/7th+8th)
 */
int getDiatonicChordNote(int switchIndex, int noteIndex) {
  int noteOffset = (pgm_read_byte(&midiNotes[switchIndex]) - diatonicRootKey + 12) % 12;
  int diatonicDegree = -1;
  for (int i = 0; i < 7; i++) {
    if (noteOffset == getModeNote(i, scaleType)) {
      diatonicDegree = i;
      break;
    }
  }
  if (diatonicDegree == -1) return -1;

  if (chordExtensionType == CHORD_EXT_TRIAD) {
    if (noteIndex > 2) return -1;
  } else if (chordExtensionType == CHORD_EXT_7TH) {
    if (noteIndex > 3) return -1;
  } else if (chordExtensionType == CHORD_EXT_7TH_OCTAVE) {
    if (noteIndex == 4) return 12;
    if (noteIndex > 4) return -1;
  }

  int degreeStep = noteIndex * 2; // 0,2,4,6
  int base = getModeNote(diatonicDegree, scaleType);
  int target = getModeNote(diatonicDegree + degreeStep, scaleType);
  return target - base;
}

/**
 * Hole eine Akkord-Note
 */
int getChordNote(int switchIndex, int variationType, int noteIndex) {
  int chordDefIndex;

  // Diatonische Modi nutzen immer das diatonische Notes-System
  if (variationType >= 0 && variationType <= 6) {
    return getDiatonicChordNote(switchIndex, noteIndex);
  }

  switch(variationType) {
    case 7:  // Power 5
      chordDefIndex = 2;
      break;
    case 8:  // Power 8
      chordDefIndex = 3;
      break;
    default:
      chordDefIndex = 0;
  }

  return pgm_read_byte(&chordDefinitions[chordDefIndex][noteIndex]);
}

// ============================================
// CHORD MODE FUNCTIONS
// ============================================

void initChordMode() {
  CLEAR_CHORD_NOTES();
  chordExtensionType = CHORD_EXT_TRIAD;
}

/**
 * Spiele Akkord-Noten basierend auf Switch
 * 
 * Input:
 *   - switchIndex: Welcher Switch (0-12)
 *   - isFolded: true=Folded Mode, false=Extended Mode
 * 
 * Output:
 *   - Aktualisiert chordModeMidiNotes[] Array mit allen Noten
 */
void playChordNotes(int switchIndex, bool isFolded = false) {
  int baseNote = pgm_read_byte(&midiNotes[switchIndex]) + (currentOctave * 12);
  
  // Berechne Akkord-Typ
  int chordDefIndex = 0;
  if (scaleType >= 0 && scaleType <= 6) {
    // Diatonische Modi
    chordDefIndex = getDiatonicChordType(switchIndex);
  } else if (scaleType == 7) {
    // Power 5
    chordDefIndex = 2;
  } else if (scaleType == 8) {
    // Power 8
    chordDefIndex = 3;
  }
  
  // Spiele jede Note des Akkords
  for (int j = 0; j < maxChordNotes; j++) {
    int noteOffset = getChordNote(switchIndex, scaleType, j);
    
    if (noteOffset >= 0) {
      int chordNote = baseNote + noteOffset;
      
      // Falte Note wenn im Folded Mode
      if (isFolded) {
        // Halte Note innerhalb der aktuellen Oktave
        while (chordNote > (currentOctave + 1) * 12) {
          chordNote -= 12;
        }
        while (chordNote < currentOctave * 12) {
          chordNote += 12;
        }
      }
      
      // Markiere Note als aktiv
      if (chordNote >= 0 && chordNote < 128) {
        SET_CHORD_NOTE_ACTIVE(chordNote, true);
      }
    }
  }
}

/**
 * Stoppe Akkord für einen Switch
 */
void stopChordNotes(int switchIndex = -1) {
  if (switchIndex >= 0) {
    // Nur für einen Switch - berechne welche Noten zu stoppen sind
    int baseNote = pgm_read_byte(&midiNotes[switchIndex]) + (currentOctave * 12);
    
    for (int j = 0; j < maxChordNotes; j++) {
      int noteOffset = getChordNote(switchIndex, scaleType, j);
      if (noteOffset >= 0) {
        int chordNote = baseNote + noteOffset;
        
        if (chordModeType == 2) {  // CHORD_MODE_FOLDED
          while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
          while (chordNote < currentOctave * 12) chordNote += 12;
        }
        
        if (chordNote >= 0 && chordNote < 128) {
          SET_CHORD_NOTE_ACTIVE(chordNote, false);
        }
      }
    }
  } else {
    // Alle Noten stoppen
    CLEAR_CHORD_NOTES();
  }
}

/**
 * Gib Array von aktiven Chord Mode Noten
 */
uint8_t* getChordModeNotes() {
  return chordModeMidiNotes;
}

/**
 * Reset all chord notes
 */
void clearChordMode() {
  stopChordNotes();
}

/**
 * CHORD MODE: turnOnChordNotes() - Komplette Implementierung
 * Schalte alle Noten eines Akkords ON
 * 
 * ABHÄNGIGKEITEN: Nutzt externe Variablen:
 * - midiNotes[], isBlackKey[], ledMapping[] (Switch-Mapping)
 * - currentOctave (aktuelle Oktave)
 * - chordModeType (EXTENDED oder FOLDED)
 * - scaleType (diatonisches Modi)
 * - activeMidiNotes[] (zum Tracking)
 * - pixels (WS2812 LEDs)
 */
void turnOnChordNotesImpl(int switchIndex, bool isFolded) {
  int baseNote = midiNotes[switchIndex] + (currentOctave * 12);
  
  // Bestimme Akkordtyp
  int chordDefIndex;
  if (scaleType >= 0 && scaleType <= 6) {
    chordDefIndex = diatonicChordPattern[(switchIndex + scaleType) % 7];
  } else if (scaleType == SCALE_POWER5) {
    chordDefIndex = 2;  // Power 5
  } else {
    chordDefIndex = 3;  // Power 8
  }
  
  for (int j = 0; j < maxChordNotes; j++) {
    int noteOffset = getChordNote(switchIndex, scaleType, j);
    if (noteOffset >= 0) {
      int chordNote = baseNote + noteOffset;
      if (isFolded) {
        while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
        while (chordNote < currentOctave * 12) chordNote += 12;
      }
      
      if (chordNote >= 0 && chordNote < 128) {
        // Send MIDI Note On
        sendMidiNote(0x90, chordNote, 0x45);
        
        // Update LED für diese Note
        int displaySwitchIndex = (chordNote == (currentOctave + 1) * 12) ? 12 : (chordNote % 12);
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          int ledIndex = pgm_read_byte(&ledMapping[displaySwitchIndex]);
          if (ledIndex >= 0) {
            uint32_t color = pgm_read_byte(&isBlackKey[displaySwitchIndex]) ? 0xFF69B4 : 0xFFFFFF;
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            pixels.setPixelColor(ledIndex, pixels.Color(r, g, b));
          }
        }
      }
    }
  }
  pixels.show();
}

/**
 * CHORD MODE: turnOffChordNotes() - Komplette Implementierung
 * Schalte alle Noten eines Akkords OFF
 */
void turnOffChordNotesImpl(int switchIndex, bool isFolded) {
  int baseNote = pgm_read_byte(&midiNotes[switchIndex]) + (currentOctave * 12);
  
  for (int j = 0; j < maxChordNotes; j++) {
    int noteOffset = getChordNote(switchIndex, scaleType, j);
    if (noteOffset >= 0) {
      int chordNote = baseNote + noteOffset;
      if (isFolded) {
        while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
        while (chordNote < currentOctave * 12) chordNote += 12;
      }
      
      if (chordNote >= 0 && chordNote < 128) {
        // Send MIDI Note Off
        sendMidiNote(0x90, chordNote, 0x00);
        
        // Update LED für diese Note
        int displaySwitchIndex = (chordNote == (currentOctave + 1) * 12) ? 12 : (chordNote % 12);
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          int ledIndex = pgm_read_byte(&ledMapping[displaySwitchIndex]);
          if (ledIndex >= 0) {
            pixels.setPixelColor(ledIndex, 0);
          }
        }
      }
    }
  }
  pixels.show();
}

#endif
