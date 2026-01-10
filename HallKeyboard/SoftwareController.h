/**
 * SOFTWARE CONTROLLER LAYER
 * 
 * Isolierte Logik für Software-State Management:
 * - Verwaltet aktive Modi (Play Mode, Chord Mode, Arpeggiator)
 * - Verwaltet Submenu Navigation
 * - Koordiniert Mode-Kombinationen
 * - Verarbeitet Function Switch Events
 * 
 * INPUT: 
 *   - switch_triggered[], switch_released[], switch_held[] vom Hardware Controller
 *   - Tap Tempo Events
 * 
 * OUTPUT:
 *   - State Variablen die andere Layer nutzen
 *   - Beschreibt welche Modi aktiv sind und wie sie zusammenwirken
 */

#ifndef SOFTWARE_CONTROLLER_H
#define SOFTWARE_CONTROLLER_H

#include "HardwareController.h"
#include "ArduinoTapTempo.h"

// Play Mode Konstanten
#define PLAY_MODE_HOLD 0
#define PLAY_MODE_ADDITIVE 1

// Chord Mode Konstanten
#define CHORD_MODE_OFF 0
#define CHORD_MODE_EXTENDED 1
#define CHORD_MODE_FOLDED 2

// Arpeggiator Konstanten
#ifndef ARPEGGIATOR_UP_DOWN
#define ARPEGGIATOR_UP_DOWN 0
#define ARPEGGIATOR_DOWN_UP 1
#define ARPEGGIATOR_UP 2
#define ARPEGGIATOR_DOWN 3
#define ARPEGGIATOR_SEQUENCE 4
#endif

// ============================================
// SOFTWARE CONTROLLER STATE
// ============================================

int currentOctave = 3;
bool isIdle = true;
int bpmPriorityBeats = 0;
extern unsigned long lastNoteActiveTime;

// Modus-Aktivierungen
bool playModeActive = false;
bool chordModeActive = false;
bool arpeggiatorActive = false;

// Submenu-Navigation
bool inSubmenu = false;
int currentSubmenu = 0;
int submenuIndex = 0;
int maxSubmenuIndex = 0;
bool submenuChanged = false;

// Play Mode Variables
int playModeType = 0;
bool holdMode = false;
bool additiveMode = false;
int heldNote = -1;
int heldSwitchIdx = -1;
bool heldNotes[NUM_SWITCHES];

// Chord Mode Variables
extern int chordModeType;
extern int scaleType;
extern int diatonicRootKey;
bool chordNotesActive[NUM_SWITCHES];

// Arpeggiator Mode Variables
extern int arpeggiatorMode;
extern int arpeggiatorRate;
bool autoHoldActivatedByArp = false;
int savedPlayModeTypeBeforeArp = 0;
bool savedPlayModeActiveBeforeArp = false;

// Save/Restore Variables
int savedArpeggiatorModeBeforeSubmenu = 0;
int savedOctaveBeforeSubmenu = 3;

// ============================================
// EXTERN GLOBALS (from HallKeyboard.ino / other headers)
// ============================================
extern int currentOctave;
extern ArduinoTapTempo tapTempo;
extern void sendMidiNote(int cmd, int pitch, int velocity);
extern void setLED(int switchIndex, bool on, bool skipLEDs = false);
extern void confirmLED(int switchIndex);
extern void showOctaveLED(int octave);
extern void disableControllerLEDsForNotes();
extern int getChordNote(int switchIndex, int variationType, int noteIndex);
extern void removeNoteFromArpeggiatorMode(int note);
extern void addNoteToArpeggiatorMode(int note);
extern void transposeArpeggiatorNotes(int semiTones);
extern void clearChordMode();
extern const int maxChordNotes;
extern bool holdModeMidiNotes[128];
extern bool isIdle;

extern int currentArpeggiatorPlayingNote;
extern bool arpeggiatorNoteIsOn;
extern int numHeldArpeggiatorNotes;
extern int currentArpeggiatorIndex;

#define NUM_SCALE_TYPES 9
#define NUM_ARPEGGIATOR_MODES 5

// ============================================
// SOFTWARE CONTROLLER: Functions
// ============================================

void initSoftwareController() {
  playModeActive = false;
  chordModeActive = false;
  arpeggiatorActive = false;
  inSubmenu = false;
  currentSubmenu = 0;
  isIdle = true;
  currentOctave = 3;
  
  for (int i = 0; i < NUM_SWITCHES; i++) {
    heldNotes[i] = false;
    chordNotesActive[i] = false;
  }
}

void enterSubmenu(int submenuNumber);
void exitSubmenu(bool saveChanges);
void handleShortPress(int fsNumber);
void togglePlayModeOnOff();
void toggleChordModeOnOff();
void toggleArpeggiatorOnOff();

// Toggle Play Mode Ein/Aus
void togglePlayModeOnOff() {
  playModeActive = !playModeActive;
  autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik
  
  if (!playModeActive) {
    // Wenn Play Mode aus, alle gehaltenen Noten sofort beenden
    for (int i = 0; i < 128; i++) {
      if (holdModeMidiNotes[i]) {
        sendMidiNote(0x90, i, 0x00);
        holdModeMidiNotes[i] = false;
      }
    }
    
    heldNote = -1;
    heldSwitchIdx = -1;
    
    for (int i = 0; i < NUM_SWITCHES; i++) {
      heldNotes[i] = false;
      setLED(i, false);
    }
    holdMode = false;
    additiveMode = false;
  } else {
    holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
    additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
  }
  
  Serial.print("Play Mode: ");
  Serial.println(playModeActive ? "EIN" : "AUS");
  
}

// Toggle Chord Mode Ein/Aus
void toggleChordModeOnOff() {
  chordModeActive = !chordModeActive;
  
  if (chordModeActive && chordModeType == CHORD_MODE_OFF) {
    chordModeType = CHORD_MODE_EXTENDED;
  }
  
  if (!chordModeActive) {
    // Wenn Chord Mode aus, beende alle per Akkord gehaltenen Noten
    // Um sicher zu gehen, beenden wir alle gehaltenen Noten im Hold Mode
    for (int i = 0; i < 128; i++) {
       if (holdModeMidiNotes[i]) {
         sendMidiNote(0x90, i, 0x00);
         holdModeMidiNotes[i] = false;
       }
    }
    
    heldNote = -1;
    heldSwitchIdx = -1;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        heldNotes[i] = false;
    }

    clearChordMode();
    for (int i = 0; i < NUM_SWITCHES; i++) {
      chordNotesActive[i] = false;
    }
  }
  
  Serial.print("Chord Mode: ");
  Serial.println(chordModeActive ? "EIN" : "AUS");
  
}

// Toggle Arpeggiator Ein/Aus
void toggleArpeggiatorOnOff() {
  arpeggiatorActive = !arpeggiatorActive;
  
  if (!arpeggiatorActive) {
    if (currentArpeggiatorPlayingNote >= 0 && currentArpeggiatorPlayingNote < 128) {
      sendMidiNote(0x80, currentArpeggiatorPlayingNote, 0);
      currentArpeggiatorPlayingNote = -1;
    }
    arpeggiatorNoteIsOn = false;
    numHeldArpeggiatorNotes = 0;

    // Wenn Auto-Hold durch Arp aktiviert wurde, jetzt wieder ausschalten
    if (autoHoldActivatedByArp) {
      playModeActive = savedPlayModeActiveBeforeArp;
      playModeType = savedPlayModeTypeBeforeArp;
      
      // Berechne hold/additive states neu basierend auf dem alten Typ
      holdMode = (playModeActive && (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE));
      additiveMode = (playModeActive && playModeType == PLAY_MODE_ADDITIVE);
      
      autoHoldActivatedByArp = false;
      
      // LEDs aufräumen
      for (int i = 0; i < NUM_SWITCHES; i++) {
        heldNotes[i] = false;
        setLED(i, false);
      }
      // Auch alle gehaltenen Noten stoppen
      for (int i = 0; i < 128; i++) {
        if (holdModeMidiNotes[i]) {
          sendMidiNote(0x90, i, 0x00);
          holdModeMidiNotes[i] = false;
        }
      }
      Serial.println("Sequence Mode Auto-Hold deaktiviert - Reset auf Vorzustand");
    }
  } else {
    currentArpeggiatorIndex = 0;
    
    // Sequence Mode Sonderfall: Aktiviere Hold (Additive) als Default
    if (arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
      if (!playModeActive && !chordModeActive) {
        // Speichere Vorzustand
        savedPlayModeActiveBeforeArp = playModeActive;
        savedPlayModeTypeBeforeArp = playModeType;
        
        playModeActive = true;
        playModeType = PLAY_MODE_ADDITIVE;
        holdMode = true;
        additiveMode = true;
        autoHoldActivatedByArp = true;
        Serial.println("Sequence Mode Auto-Hold (Additive) aktiviert");
      }
    }
  }
  
  Serial.print("Arpeggiator Mode: ");
  Serial.println(arpeggiatorActive ? "EIN" : "AUS");
  
}

// Submenu betreten
void enterSubmenu(int submenuNumber) {
  inSubmenu = true;
  currentSubmenu = submenuNumber;
  submenuIndex = 0;
  submenuChanged = true;
  isIdle = true;
  
  switch(submenuNumber) {
    case 1:
      maxSubmenuIndex = 2;
      submenuIndex = playModeType;
      break;
    case 2:
      maxSubmenuIndex = NUM_SCALE_TYPES;
      submenuIndex = scaleType;
      break;
    case 3:
      maxSubmenuIndex = NUM_ARPEGGIATOR_MODES;
      submenuIndex = arpeggiatorMode;
      savedArpeggiatorModeBeforeSubmenu = arpeggiatorMode;
      break;
    case 4:
      maxSubmenuIndex = 8;
      submenuIndex = currentOctave;
      savedOctaveBeforeSubmenu = currentOctave;
      break;
  }
  
  Serial.print("Submenu ");
  Serial.print(submenuNumber);
  Serial.println(" geöffnet");
  
}

// Submenu verlassen
void exitSubmenu(bool saveChanges) {
  if (saveChanges) {
    switch(currentSubmenu) {
      case 1:
        if (submenuIndex != playModeType) {
          playModeType = submenuIndex;
          autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik
          if (playModeActive) {
            holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
            additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
          }
        }
        break;
      case 2:
        if (submenuIndex != scaleType) {
          scaleType = submenuIndex;
        }
        break;
      case 3:
        if (submenuIndex != arpeggiatorMode) {
          int oldArpMode = arpeggiatorMode;
          arpeggiatorMode = submenuIndex;
          
          // Sequence Mode Sonderfall: Aktiviere Hold (Additive) als Default
          if (arpeggiatorActive && arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
            if (!playModeActive && !chordModeActive) {
              // Speichere Vorzustand
              savedPlayModeActiveBeforeArp = playModeActive;
              savedPlayModeTypeBeforeArp = playModeType;

              playModeActive = true;
              playModeType = PLAY_MODE_ADDITIVE;
              holdMode = true;
              additiveMode = true;
              autoHoldActivatedByArp = true;
              Serial.println("Sequence Mode Auto-Hold (Additive) aktiviert");
            }
          }
          // Wenn wir von Sequence wegwechseln, Auto-Hold ggf. deaktivieren
          else if (arpeggiatorActive && oldArpMode == ARPEGGIATOR_SEQUENCE && autoHoldActivatedByArp) {
              playModeActive = savedPlayModeActiveBeforeArp;
              playModeType = savedPlayModeTypeBeforeArp;
              
              // Berechne hold/additive states neu basierend auf dem alten Typ
              holdMode = (playModeActive && (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE));
              additiveMode = (playModeActive && playModeType == PLAY_MODE_ADDITIVE);
              
              autoHoldActivatedByArp = false;
              
              for (int i = 0; i < NUM_SWITCHES; i++) {
                heldNotes[i] = false;
              }
              for (int i = 0; i < 128; i++) {
                if (holdModeMidiNotes[i]) {
                  sendMidiNote(0x90, i, 0x00);
                  holdModeMidiNotes[i] = false;
                }
              }
              Serial.println("Sequence Mode Auto-Hold zurückgesetzt auf Vorzustand");
          }
        }
        break;
      case 4:
        showOctaveLED(currentOctave);
        break;
    }
  } else {
    switch(currentSubmenu) {
      case 3:
        arpeggiatorMode = savedArpeggiatorModeBeforeSubmenu;
        
        break;
      case 4:
        transposeArpeggiatorNotes((savedOctaveBeforeSubmenu - currentOctave) * 12);
        currentOctave = savedOctaveBeforeSubmenu;
        break;
    }
  }
  
  inSubmenu = false;
  currentSubmenu = 0;
  submenuIndex = 0;
  submenuChanged = true;
  Serial.println("Submenu verlassen");
  
}

// Short-Press Handler
void handleShortPress(int fsNumber) {
  // Sofortiger Wechsel zum Control Layer bei jedem FS-Druck
  lastNoteActiveTime = 0; 
  bpmPriorityBeats = 0;

  if (inSubmenu) {
    switch(fsNumber) {
      case 1: exitSubmenu(false); break;
      case 2: exitSubmenu(true); break;
      case 3:
        if (submenuIndex > 0) {
          submenuIndex--;
          submenuChanged = true;
          if (currentSubmenu == 4) {
            currentOctave = submenuIndex;
            transposeArpeggiatorNotes(-12);
          }
        }
        break;
      case 4:
        if (submenuIndex < maxSubmenuIndex - 1) {
          submenuIndex++;
          submenuChanged = true;
          if (currentSubmenu == 4) {
            currentOctave = submenuIndex;
            transposeArpeggiatorNotes(12);
          }
        }
        break;
    }
  } else {
    switch(fsNumber) {
      case 1: togglePlayModeOnOff(); break;
      case 2: toggleChordModeOnOff(); break;
      case 3: toggleArpeggiatorOnOff(); break;
      case 4:
        Serial.print("Tap Tempo registriert - BPM: ");
        Serial.println(tapTempo.getBPM());
        break;
    }
  }
}

/**
 * Verarbeite Function Switch Events
 */
void handleFunctionSwitches() {
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    if (functionSwitches[i].trigger()) {
      functionSwitchPressTime[i] = millis();
      functionSwitchLongPressed[i] = false;
    }
    
    if (functionSwitches[i].isDown() && !inSubmenu && !functionSwitchLongPressed[i]) {
      unsigned long currentPressTime = millis() - functionSwitchPressTime[i];
      if (currentPressTime >= LONG_PRESS_DURATION) {
        functionSwitchLongPressed[i] = true;
        enterSubmenu(i + 1);
      }
    }
    
    if (functionSwitches[i].released()) {
      if (!functionSwitchLongPressed[i]) {
        handleShortPress(i + 1);
      }
      functionSwitchLongPressed[i] = false;
    }
  }
}

/**
 * Zentrales Update des Software Controllers
 */
void updateSoftwareController() {
  handleFunctionSwitches();
}

/**
 * Prozessiert alle Note-Switches (die Kern-Logik aus loop())
 */
void processNoteSwitches() {
  for (int i = 0; i < NUM_SWITCHES; i++) {
    int currentNote = getHardwareMIDINote(i);
    
    if (switch_triggered[i]) {
      // Submenu handling
      if (inSubmenu) {
        // Beim Oktave-Wechsel (Submenu 4) Noten normal weiterspielen lassen!
        if (currentSubmenu == 4) {
          // Keine spezielle Sperre für Arpeggiator oder Hold hier
        } else if (currentSubmenu == 1 || currentSubmenu == 3) {
          sendMidiNote(0x90, currentNote, 0x45);
          continue;
        } else if (currentSubmenu == 2) {
          diatonicRootKey = midiNotes[i];
          confirmLED(i);
          continue;
        }
      }
      
      disableControllerLEDsForNotes();
      setLED(i, true);
      
      // Calculate notes to play
      int notesToPlay[3];
      int numNotesToPlay = 1;
      
      if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
        bool isFolded = (chordModeType == CHORD_MODE_FOLDED);
        int baseNote = midiNotes[i] + (currentOctave * 12);
        numNotesToPlay = 0;
        for (int j = 0; j < maxChordNotes; j++) {
          int noteOffset = getChordNote(i, scaleType, j);
          if (noteOffset >= 0 && numNotesToPlay < 3) {
            int chordNote = baseNote + noteOffset;
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
      
      // HOLD+ARP SPECIAL: Remove old switch notes (Nur im Mono-Hold)
      if (holdMode && arpeggiatorActive && !additiveMode && heldSwitchIdx != -1 && heldSwitchIdx != i) {
        int oldBaseNote = midiNotes[heldSwitchIdx] + (currentOctave * 12);
        if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
          bool isFoldedCheck = (chordModeType == CHORD_MODE_FOLDED);
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
          // Neuer Switch im Single Hold: Alle alten Noten aus!
          if (heldSwitchIdx != -1) {
            heldNotes[heldSwitchIdx] = false;
            for (int n = 0; n < 128; n++) {
              if (holdModeMidiNotes[n]) {
                sendMidiNote(0x90, n, 0x00);
                holdModeMidiNotes[n] = false;
              }
            }
          }
          heldSwitchIdx = i;
          heldNotes[i] = true;
          isTriggeringNew = true;
        } else if (holdMode && isSameSwitchDouble) {
          // Gleicher Switch nochmal: Alles aus!
          heldSwitchIdx = -1;
          heldNotes[i] = false;
          for (int n = 0; n < 128; n++) {
            if (holdModeMidiNotes[n]) {
              sendMidiNote(0x90, n, 0x00);
              holdModeMidiNotes[n] = false;
            }
          }
          isTriggeringNew = false;
        } else {
          isTriggeringNew = true;
        }
      }
      
      // Play notes
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
            // Single Hold: Note aktivieren (Alte wurden oben bereits deaktiviert)
            if (isTriggeringNew) {
              holdModeMidiNotes[noteToPlay] = true;
              sendMidiNote(0x90, noteToPlay, 0x45);
            }
          }
        } else if (arpeggiatorActive) {
          addNoteToArpeggiatorMode(noteToPlay);
        } else {
          sendMidiNote(0x90, noteToPlay, 0x45);
        }
      }
      
      if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
        chordNotesActive[i] = true;
      }
    }
    
    if (switch_released[i]) {
      if (inSubmenu && (currentSubmenu == 1 || currentSubmenu == 3 || currentSubmenu == 4)) {
        if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
          int baseNote = midiNotes[i] + (currentOctave * 12);
          for (int j = 0; j < maxChordNotes; j++) {
            int noteOffset = getChordNote(i, scaleType, j);
            if (noteOffset >= 0) {
              int chordNote = baseNote + noteOffset;
              if (chordModeType == CHORD_MODE_FOLDED) {
                while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
                while (chordNote < currentOctave * 12) chordNote += 12;
              }
              sendMidiNote(0x90, chordNote, 0x00);
            }
          }
        } else {
          sendMidiNote(0x90, currentNote, 0x00);
        }
      } else {
        int notesToRelease[3];
        int numNotesToRelease = 1;
        
        if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
          bool isFolded = (chordModeType == CHORD_MODE_FOLDED);
          int baseNote = midiNotes[i] + (currentOctave * 12);
          numNotesToRelease = 0;
          for (int j = 0; j < maxChordNotes; j++) {
            int noteOffset = getChordNote(i, scaleType, j);
            if (noteOffset >= 0 && numNotesToRelease < 3) {
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
          if (holdMode && arpeggiatorActive) { }
          else if (holdMode) { }
          else if (arpeggiatorActive) removeNoteFromArpeggiatorMode(noteToRelease);
          else sendMidiNote(0x90, noteToRelease, 0x00);
        }
        
        if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
          chordNotesActive[i] = false;
        }
      }
      setLED(i, false);
    }
  }
}

#endif

