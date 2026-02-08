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

// Play Mode Konstanten (Toggle-Modi für FS1)
#define PLAY_MODE_TOGGLE_HOLD_ADDITIVE 0
#define PLAY_MODE_TOGGLE_OFF_HOLD 1
#define PLAY_MODE_TOGGLE_OFF_ADDITIVE 2

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

// Arpeggiator Rate Konstanten
#ifndef RATE_QUARTER
#define RATE_WHOLE 0
#define RATE_QUARTER 1
#define RATE_EIGHTH 2
#define RATE_TRIPLET 3
#define RATE_SIXTEENTH 4
#endif

// ============================================
// SOFTWARE CONTROLLER STATE
// ============================================

int8_t currentOctave = 3;
bool isIdle = true;
uint8_t bpmPriorityBeats = 0;
extern unsigned long lastNoteActiveTime;

// Modus-Aktivierungen
bool playModeActive = false;
bool chordModeActive = false;
bool arpeggiatorActive = false;

// Submenu-Navigation
bool inSubmenu = false;
int8_t currentSubmenu = 0;
int8_t currentSubmenuPage = 0;
int8_t submenuIndex = 0;
int8_t maxSubmenuIndex = 0;
bool submenuChanged = false;

// Play Mode Variables
uint8_t playModeType = PLAY_MODE_TOGGLE_HOLD_ADDITIVE;
bool holdMode = false;
bool additiveMode = false;
int8_t heldNote = -1;
int8_t heldSwitchIdx = -1;
bool heldNotes[NUM_SWITCHES];
uint8_t activeSwitchNotes[NUM_SWITCHES][5];
uint8_t activeSwitchNumNotes[NUM_SWITCHES];

// Chord Mode Variables
extern int8_t chordModeType;
extern int8_t scaleType;
extern int8_t diatonicRootKey;
extern uint8_t chordExtensionType;
bool chordNotesActive[NUM_SWITCHES];

// Arpeggiator Mode Variables
extern int8_t arpeggiatorMode;
extern uint8_t arpeggiatorRate;
extern uint8_t arpeggiatorDutyCycle;
bool autoHoldActivatedByArp = false;
bool savedAdditiveModeBeforeArp = false;
bool savedPlayModeActiveBeforeArp = false;

// Forward Declarations für Arpeggiator Synchronisation
extern void resetArpeggiatorPhase();
extern bool arpWaitingForSync;

// New: Reference counting for overlapping chords in Additive Hold
uint8_t holdModeNoteRefCount[128];

// Save/Restore Variables
int8_t savedArpeggiatorModeBeforeSubmenu = 0;
uint8_t savedArpeggiatorRateBeforeSubmenu = 2; // Default RATE_EIGHTH
uint8_t savedArpeggiatorDutyCycleBeforeSubmenu = 50;
int8_t savedOctaveBeforeSubmenu = 3;
bool savedPlayModeActiveBeforeSubmenu = false;
bool savedChordModeActiveBeforeSubmenu = false;
bool savedArpeggiatorActiveBeforeSubmenu = false;

// ============================================
// EXTERN GLOBALS (from HallKeyboard.ino / other headers)
// ============================================
extern int8_t currentOctave;
extern ArduinoTapTempo tapTempo;
extern volatile bool midiClockActive;
extern uint16_t calculatedBPM;
extern void syncMidiClockPhase();
extern void syncMidiClockToBPM();
extern uint8_t bpmPriorityBeats;
extern void sendMidiNote(int cmd, int pitch, int velocity);
extern void setLED(int switchIndex, bool on, bool skipLEDs = false);
extern void confirmLED(int switchIndex);
extern void disableControllerLEDsForNotes();
extern bool switch_triggered[13];
extern bool switch_released[13];
extern bool switch_held[13];
extern int getChordNote(int switchIndex, int variationType, int noteIndex);
extern void removeNoteFromArpeggiatorMode(int note);
extern void addNoteToArpeggiatorMode(int note);
extern void clearArpeggiatorNotes();
extern void transposeArpeggiatorNotes(int semiTones);
extern uint8_t arpNoteRefCount[128];
extern void clearChordMode();
extern const uint8_t maxChordNotes;
extern uint8_t holdModeMidiNotes[16];
extern bool isIdle;

#define IS_HOLD_NOTE_ACTIVE(n) ((holdModeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_HOLD_NOTE_ACTIVE(n, v) if(v) holdModeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else holdModeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))

extern int8_t currentArpeggiatorPlayingNote;
extern bool arpeggiatorNoteIsOn;
extern int8_t numHeldArpeggiatorNotes;
extern int8_t currentArpeggiatorIndex;

#define NUM_SCALE_TYPES 9
#define NUM_ARPEGGIATOR_MODES 5

// ============================================
// SOFTWARE CONTROLLER: Functions
// ============================================

void initSoftwareController() {
  playModeType = PLAY_MODE_TOGGLE_HOLD_ADDITIVE;
  playModeActive = true;
  chordModeActive = false;
  arpeggiatorActive = false;
  inSubmenu = false;
  currentSubmenu = 0;
  isIdle = true;
  currentOctave = 3;
  holdMode = true;
  additiveMode = false;
  heldNote = -1;
  heldSwitchIdx = -1;
  
  for (int i = 0; i < NUM_SWITCHES; i++) {
    heldNotes[i] = false;
    chordNotesActive[i] = false;
    activeSwitchNumNotes[i] = 0;
  }
  for (int i = 0; i < 128; i++) {
    holdModeNoteRefCount[i] = 0;
  }
}

void enterSubmenu(int submenuNumber);
void exitSubmenu(bool saveChanges);
void handleShortPress(int fsNumber);
void togglePlayModeOnOff();
void toggleChordModeOnOff();
void toggleArpeggiatorOnOff();

// Play Mode sicher deaktivieren (inkl. Cleanup)
void deactivatePlayMode() {
  playModeActive = false;
  autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik

  // Wenn Play Mode aus, alle gehaltenen Noten sofort beenden
  for (int i = 0; i < 128; i++) {
    holdModeNoteRefCount[i] = 0;
    if (IS_HOLD_NOTE_ACTIVE(i)) {
      sendMidiNote(0x90, i, 0x00);
      SET_HOLD_NOTE_ACTIVE(i, false);
      // BUG FIX: Wenn Hold deaktiviert wird, Noten auch aus Arp entfernen (falls sie nicht physikalisch gehalten werden)
      removeNoteFromArpeggiatorMode(i);
    }
  }

  heldNote = -1;
  heldSwitchIdx = -1;

  for (int i = 0; i < NUM_SWITCHES; i++) {
    heldNotes[i] = false;
    activeSwitchNumNotes[i] = 0;
    setLED(i, false);
  }

  holdMode = false;
  additiveMode = false;
}

// Toggle Play Mode Ein/Aus (neuer Standard)
void togglePlayModeOnOff() {
  autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik

  switch (playModeType) {
    case PLAY_MODE_TOGGLE_HOLD_ADDITIVE:
      if (!playModeActive) {
        playModeActive = true;
        holdMode = true;
        additiveMode = false;
      } else {
        holdMode = true;
        additiveMode = !additiveMode; // HOLD <-> ADDITIVE HOLD
      }
      break;

    case PLAY_MODE_TOGGLE_OFF_HOLD:
      if (playModeActive) {
        deactivatePlayMode();
      } else {
        playModeActive = true;
        holdMode = true;
        additiveMode = false;
      }
      break;

    case PLAY_MODE_TOGGLE_OFF_ADDITIVE:
      if (playModeActive) {
        deactivatePlayMode();
      } else {
        playModeActive = true;
        holdMode = true;
        additiveMode = true;
      }
      break;

    default:
      // Fallback: HOLD <-> ADDITIVE HOLD
      if (!playModeActive) {
        playModeActive = true;
        holdMode = true;
        additiveMode = false;
      } else {
        holdMode = true;
        additiveMode = !additiveMode;
      }
      break;
  }
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
       holdModeNoteRefCount[i] = 0;
       if (IS_HOLD_NOTE_ACTIVE(i)) {
         sendMidiNote(0x90, i, 0x00);
         SET_HOLD_NOTE_ACTIVE(i, false);
         // Auch aus Arp entfernen
         removeNoteFromArpeggiatorMode(i);
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
  
  //Serial.print("Chord Mode: ");
  //Serial.println(chordModeActive ? "EIN" : "AUS");
  
}

// Toggle Arpeggiator Ein/Aus
void toggleArpeggiatorOnOff() {
  arpeggiatorActive = !arpeggiatorActive;
  
  if (arpeggiatorActive) {
    // Neu: Warte auf den nächsten Downbeat (1)
    arpWaitingForSync = true;
    
    // Bestehende Arp-Noten sicherheitshalber löschen (Neustart von Clean Slate)
    clearArpeggiatorNotes();
    
    // Nur MIDI START/Phase Reset senden, wenn wir MASTER sind (keine externe Clock)
    if (!midiClockActive) {
      startMidiClock();
      tapTempo.resetTapChain(); // Setzt Master-Beat auf "jetzt"
      resetArpeggiatorPhase();  // Setzt Arp-Trigger-Logik zurück
    }
    
    // 1. Statische Hold-Noten stoppen, wenn sie gespielt werden
    if (holdMode) {
      for (int i = 0; i < 128; i++) {
        if (IS_HOLD_NOTE_ACTIVE(i)) {
          sendMidiNote(0x90, i, 0x00);
        }
      }
    }
    
    // 2. Bestehende gehaltene Noten (aus manuellem Hold) in den Arpeggiator übertragen
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (heldNotes[i] || switch_held[i]) {
            // Berechne Noten (inkl. Chords)
            int baseNote = pgm_read_byte(&midiNotes[i]) + (currentOctave * 12);
            if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
                bool isFolded = (chordModeType == CHORD_MODE_FOLDED);
                for (int j = 0; j < maxChordNotes; j++) {
                    int noteOffset = getChordNote(i, scaleType, j);
                    if (noteOffset >= 0) {
                        int chordNote = baseNote + noteOffset;
                        if (isFolded) {
                            while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
                            while (chordNote < currentOctave * 12) chordNote += 12;
                        }
                        addNoteToArpeggiatorMode(chordNote);
                    }
                }
            } else {
                addNoteToArpeggiatorMode(baseNote);
            }
        }
    }

    currentArpeggiatorIndex = -1; // -1 sorgt dafür, dass er beim nächsten Beat bei 0 startet
    
    // ARP Modus: Hold immer aktivieren (Default Latch / Play Mode 0)
    if (!playModeActive) {
      // Speichere Vorzustand
      savedPlayModeActiveBeforeArp = playModeActive;
      savedAdditiveModeBeforeArp = additiveMode;

      playModeActive = true;
      // Bei Sequence Mode erzwingen wir Additive, sonst Latch (Hold)
      if (arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
        additiveMode = true;
      } else {
        additiveMode = false;
      }

      holdMode = true;
      autoHoldActivatedByArp = true;
    }
  } else {
    // Arpeggiator ausschalten
    
    // MIDI Clock Synchronisation: STOP (wenn konfiguriert)
    if (stopClockOnArpDeactivate) {
      stopMidiClock();
    }

    // 1. Arpeggiator stoppen und Speicher leeren
    clearArpeggiatorNotes();
    
    // Wenn Auto-Hold durch Arp aktiviert wurde, jetzt wieder ausschalten
    if (autoHoldActivatedByArp) {
      playModeActive = savedPlayModeActiveBeforeArp;
      additiveMode = savedAdditiveModeBeforeArp;

      // Berechne hold/additive states neu basierend auf dem alten Zustand
      holdMode = playModeActive;
      if (!playModeActive) {
        additiveMode = false;
      }

      autoHoldActivatedByArp = false;

      // LEDs aufräumen
      for (int i = 0; i < NUM_SWITCHES; i++) {
        heldNotes[i] = false;
        setLED(i, false);
      }
      // Auch alle gehaltenen Noten stoppen und RefCounts zurücksetzen
      for (int i = 0; i < 128; i++) {
        holdModeNoteRefCount[i] = 0;
        if (IS_HOLD_NOTE_ACTIVE(i)) {
          sendMidiNote(0x90, i, 0x00);
          SET_HOLD_NOTE_ACTIVE(i, false);
        }
      }
    }
  }
}

void enterSubmenuPage(int submenuNumber, int page);

// Submenu betreten
void enterSubmenu(int submenuNumber) {
  inSubmenu = true;
  currentSubmenu = submenuNumber;
  currentSubmenuPage = 0;
  submenuChanged = true;
  isIdle = true;
  
  // Merk dir den aktiven Status vor dem Öffnen
  if (submenuNumber == 1) savedPlayModeActiveBeforeSubmenu = playModeActive;
  if (submenuNumber == 2) savedChordModeActiveBeforeSubmenu = chordModeActive;
  if (submenuNumber == 3) savedArpeggiatorActiveBeforeSubmenu = arpeggiatorActive;

  // Modus automatisch aktivieren, falls nicht an
  if (submenuNumber == 1 && !playModeActive) togglePlayModeOnOff();
  if (submenuNumber == 2 && !chordModeActive) toggleChordModeOnOff();
  if (submenuNumber == 3 && !arpeggiatorActive) toggleArpeggiatorOnOff();

  enterSubmenuPage(submenuNumber, 0);
  
  //Serial.print("Submenu ");
  //Serial.print(submenuNumber);
  //Serial.println(" geöffnet");
}

void enterSubmenuPage(int submenuNumber, int page) {
  submenuIndex = 0;
  currentSubmenuPage = page;
  
  switch(submenuNumber) {
    case 1:
      maxSubmenuIndex = 3;
      submenuIndex = playModeType;
      break;
    case 2:
      if (page == 0) {
        maxSubmenuIndex = NUM_SCALE_TYPES;
        submenuIndex = scaleType;
      } else if (page == 1) {
        maxSubmenuIndex = 2; // Extended, Folded
        submenuIndex = (chordModeType == CHORD_MODE_FOLDED) ? 1 : 0;
      } else {
        maxSubmenuIndex = 3; // Triad, 7th, 7th+8th
        submenuIndex = chordExtensionType;
      }
      break;
    case 3:
      if (page == 0) {
        maxSubmenuIndex = NUM_ARPEGGIATOR_MODES;
        submenuIndex = arpeggiatorMode;
        savedArpeggiatorModeBeforeSubmenu = arpeggiatorMode;
        savedArpeggiatorRateBeforeSubmenu = arpeggiatorRate;
        savedArpeggiatorDutyCycleBeforeSubmenu = arpeggiatorDutyCycle;
      } else if (page == 1) {
        maxSubmenuIndex = 5; // WHOLE, QUARTER, EIGHTH, TRIPLET, SIXTEENTH
        // Mapping: 0->WHOLE, 1->QUARTER, 2->EIGHTH, 3->TRIPLET, 4->SIXTEENTH
        if (arpeggiatorRate == RATE_WHOLE) submenuIndex = 0;
        else if (arpeggiatorRate == RATE_QUARTER) submenuIndex = 1;
        else if (arpeggiatorRate == RATE_EIGHTH) submenuIndex = 2;
        else if (arpeggiatorRate == RATE_TRIPLET) submenuIndex = 3;
        else if (arpeggiatorRate == RATE_SIXTEENTH) submenuIndex = 4;
      } else {
        // Page 2: Duty Cycle
        maxSubmenuIndex = 8;
        int dC = arpeggiatorDutyCycle;
        if (dC <= 10) submenuIndex = 0;
        else if (dC <= 25) submenuIndex = 1;
        else if (dC <= 40) submenuIndex = 2;
        else if (dC <= 50) submenuIndex = 3;
        else if (dC <= 60) submenuIndex = 4;
        else if (dC <= 75) submenuIndex = 5;
        else if (dC <= 90) submenuIndex = 6;
        else submenuIndex = 7;
      }
      break;
    case 4:
      maxSubmenuIndex = 8;
      submenuIndex = currentOctave;
      savedOctaveBeforeSubmenu = currentOctave;
      break;
  }
  submenuChanged = true;
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
            holdMode = true;
            if (playModeType == PLAY_MODE_TOGGLE_OFF_HOLD) {
              additiveMode = false;
            } else if (playModeType == PLAY_MODE_TOGGLE_OFF_ADDITIVE) {
              additiveMode = true;
            }
          } else {
            holdMode = false;
            additiveMode = false;
          }
        }
        break;
      case 2:
        if (currentSubmenuPage == 0) {
          if (submenuIndex != scaleType) {
            scaleType = submenuIndex;
          }
        } else if (currentSubmenuPage == 1) {
          chordModeType = (submenuIndex == 1) ? CHORD_MODE_FOLDED : CHORD_MODE_EXTENDED;
        } else if (currentSubmenuPage == 2) {
          chordExtensionType = submenuIndex;
        }
        break;
      case 3:
        if (currentSubmenuPage == 0) {
          if (submenuIndex != arpeggiatorMode) {
            int oldArpMode = arpeggiatorMode;
            arpeggiatorMode = submenuIndex;
            
            // Sequence Mode Sonderfall: Aktiviere Additive (Hold ist eh an)
            if (arpeggiatorActive && arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
                playModeActive = true;
                holdMode = true;
                additiveMode = true;
            }
          }
        } else if (currentSubmenuPage == 1) {
          // Rate Mapping
          int rates[] = {RATE_WHOLE, RATE_QUARTER, RATE_EIGHTH, RATE_TRIPLET, RATE_SIXTEENTH};
          arpeggiatorRate = rates[submenuIndex % 5];
        } else if (currentSubmenuPage == 2) {
          // Duty Cycle Mapping
          int duties[] = {10, 25, 40, 50, 60, 75, 90, 99};
          arpeggiatorDutyCycle = duties[submenuIndex % 8];
        }
        break;
    }
  } else {
    // Falls der Modus erst beim Öffnen des Submenüs aktiviert wurde, beim Abbrechen wieder deaktivieren
    if (currentSubmenu == 1 && playModeActive != savedPlayModeActiveBeforeSubmenu) {
      togglePlayModeOnOff();
    }
    if (currentSubmenu == 2 && chordModeActive != savedChordModeActiveBeforeSubmenu) {
      toggleChordModeOnOff();
    }
    if (currentSubmenu == 3 && arpeggiatorActive != savedArpeggiatorActiveBeforeSubmenu) {
      toggleArpeggiatorOnOff();
    }

    switch(currentSubmenu) {
      case 3:
        arpeggiatorMode = savedArpeggiatorModeBeforeSubmenu;
        arpeggiatorRate = savedArpeggiatorRateBeforeSubmenu;
        arpeggiatorDutyCycle = savedArpeggiatorDutyCycleBeforeSubmenu;
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
  //Serial.println("Submenu verlassen");
  
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
          
          // Real-time Preview Logik
          if (currentSubmenu == 3) {
            if (currentSubmenuPage == 0) {
              arpeggiatorMode = submenuIndex;
            } else if (currentSubmenuPage == 1) {
              int rates[] = {RATE_WHOLE, RATE_QUARTER, RATE_EIGHTH, RATE_TRIPLET, RATE_SIXTEENTH};
              if (submenuIndex >= 0 && submenuIndex < 5) {
                arpeggiatorRate = rates[submenuIndex];
              }
            } else if (currentSubmenuPage == 2) {
              int duties[] = {10, 25, 40, 50, 60, 75, 90, 99};
              if (submenuIndex >= 0 && submenuIndex < 8) {
                arpeggiatorDutyCycle = duties[submenuIndex];
              }
            }
          }
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
          
          // Real-time Preview Logik
          if (currentSubmenu == 3) {
            if (currentSubmenuPage == 0) {
              arpeggiatorMode = submenuIndex;
            } else if (currentSubmenuPage == 1) {
              int rates[] = {RATE_WHOLE, RATE_QUARTER, RATE_EIGHTH, RATE_TRIPLET, RATE_SIXTEENTH};
              if (submenuIndex >= 0 && submenuIndex < 5) {
                arpeggiatorRate = rates[submenuIndex];
              }
            } else if (currentSubmenuPage == 2) {
              int duties[] = {10, 25, 40, 50, 60, 75, 90, 99};
              if (submenuIndex >= 0 && submenuIndex < 8) {
                arpeggiatorDutyCycle = duties[submenuIndex];
              }
            }
          }
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
        if (midiClockActive) {
          // NEU: Hochpräziser Phase-Reset basierend auf dem BUTTON DOWN Zeitpunkt
          // Wir kompensieren die Zeit, die der User den Knopf gehalten hat.
          unsigned long deltaMicros = micros() - functionSwitchPressMicros[3];
          
          // Dauer eines einzelnen MIDI-Pulses in Mikrosekunden
          float pulseLenMicros = (60000000.0 / (float)calculatedBPM) / 24.0;
          
          // Berechne wie viele Pulse seit dem Niederdrücken vergangen sind
          uint16_t pulsesSinceDown = (uint16_t)(deltaMicros / pulseLenMicros);
          
          // Setze die globale Master-Phase so, dass der DOWN-Event exakt die "1" war.
          // Wir addieren pulsesSinceDown, da wir jetzt "pulsesSinceDown" nach der 1 sind.
          extern volatile uint16_t masterPulseCounter;
          extern volatile uint16_t ppqnCounter;
          masterPulseCounter = pulsesSinceDown % 96;
          ppqnCounter = pulsesSinceDown % 24;

          // Arpeggiator stoppen und auf den nächsten Taktanfang (die jetzt korrigierte 1) warten
          resetArpeggiatorPhase();
          arpWaitingForSync = true;
        } else {
          // Tap Tempo Downbeat Sync bei interner Clock
          bpmPriorityBeats = 8;
          syncMidiClockToBPM();
        }
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
      functionSwitchPressMicros[i] = micros();
      functionSwitchLongPressed[i] = false;
    }
    
    if (functionSwitches[i].isDown() && !functionSwitchLongPressed[i]) {
      unsigned long currentPressTime = millis() - functionSwitchPressTime[i];
      if (currentPressTime >= LONG_PRESS_DURATION) {
        functionSwitchLongPressed[i] = true;
        
        if (inSubmenu) {
          // Innerhalb eines Submenüs: FS4 (Index 3) blättert Seiten um
          if (i == 3) {
            int numPages = 1; // Default: 1 Seite (0)
            if (currentSubmenu == 2) numPages = 3;
            if (currentSubmenu == 3) numPages = 3;
            
            currentSubmenuPage = (currentSubmenuPage + 1) % numPages;
            enterSubmenuPage(currentSubmenu, currentSubmenuPage);
            //Serial.print("Submenu Page gewechselt zu: ");
            //Serial.println(currentSubmenuPage);
          }
        } else {
          // SPECIAL: Long Press on Hold (FS1) in ARP Mode clears ARP memory
          if (i == 0 && arpeggiatorActive) {
            clearArpeggiatorNotes();
            
            // Auch Hold-Noten im Speicher löschen (damit der Arp wirklich leer ist)
            for (int n = 0; n < 128; n++) {
              holdModeNoteRefCount[n] = 0;
              if (IS_HOLD_NOTE_ACTIVE(n)) {
                SET_HOLD_NOTE_ACTIVE(n, false);
              }
            }
            for (int s = 0; s < NUM_SWITCHES; s++) {
               heldNotes[s] = false;
            }
            heldNote = -1;
            heldSwitchIdx = -1;

            confirmLED(0); // Visuelles Feedback
          } else {
            enterSubmenu(i + 1);
          }
        }
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
          activeSwitchNotes[i][0] = currentNote;
          activeSwitchNumNotes[i] = 1;
          continue;
        } else if (currentSubmenu == 2) {
          diatonicRootKey = pgm_read_byte(&midiNotes[i]);
          confirmLED(i);
          continue;
        }
      }
      
      disableControllerLEDsForNotes();
      setLED(i, true);
      
      // Calculate notes to play
      int notesToPlay[5];
      int numNotesToPlay = 1;
      
      if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
        bool isFolded = (chordModeType == CHORD_MODE_FOLDED);
        int baseNote = pgm_read_byte(&midiNotes[i]) + (currentOctave * 12);
        numNotesToPlay = 0;
        for (int j = 0; j < maxChordNotes; j++) {
          int noteOffset = getChordNote(i, scaleType, j);
          if (noteOffset >= 0 && numNotesToPlay < 5) {
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
        int oldBaseNote = pgm_read_byte(&midiNotes[heldSwitchIdx]) + (currentOctave * 12);
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

        if (isTriggeringNew) {
          activeSwitchNumNotes[i] = numNotesToPlay;
          for (int n = 0; n < numNotesToPlay; n++) {
            activeSwitchNotes[i][n] = notesToPlay[n];
          }
        } else {
          // Beim Toggle-Ausschalten im Additiven Hold: Nutze gespeicherte Noten
          numNotesToPlay = activeSwitchNumNotes[i];
          for (int n = 0; n < numNotesToPlay; n++) {
            notesToPlay[n] = activeSwitchNotes[i][n];
          }
          activeSwitchNumNotes[i] = 0;
        }
      } else {
        bool isSameSwitchDouble = (heldSwitchIdx == i);
        if (holdMode && !isSameSwitchDouble) {
          // Neuer Switch im Single Hold: Alle alten Noten aus!
          if (heldSwitchIdx != -1) {
            heldNotes[heldSwitchIdx] = false;
            activeSwitchNumNotes[heldSwitchIdx] = 0; 
            for (int n = 0; n < 128; n++) {
              holdModeNoteRefCount[n] = 0;
              if (IS_HOLD_NOTE_ACTIVE(n)) {
                if (!arpeggiatorActive) sendMidiNote(0x90, n, 0x00);
                SET_HOLD_NOTE_ACTIVE(n, false);
                removeNoteFromArpeggiatorMode(n);
              }
            }
          }
          heldSwitchIdx = i;
          heldNotes[i] = true;
          isTriggeringNew = true;
          
          // Speichern für Single Hold (auch wenn wir hier den RefCount-Quickfix oben haben)
          activeSwitchNumNotes[i] = numNotesToPlay;
          for (int n = 0; n < numNotesToPlay; n++) {
            activeSwitchNotes[i][n] = notesToPlay[n];
          }
        } else if (holdMode && isSameSwitchDouble) {
          // Gleicher Switch nochmal: Alles aus!
          heldSwitchIdx = -1;
          heldNotes[i] = false;
          
          // Nutze gespeicherte Noten für das Ausschalten
          numNotesToPlay = activeSwitchNumNotes[i];
          for (int n = 0; n < numNotesToPlay; n++) {
            notesToPlay[n] = activeSwitchNotes[i][n];
          }
          activeSwitchNumNotes[i] = 0;

          for (int n = 0; n < 128; n++) {
            holdModeNoteRefCount[n] = 0;
            if (IS_HOLD_NOTE_ACTIVE(n)) {
              if (!arpeggiatorActive) sendMidiNote(0x90, n, 0x00);
              SET_HOLD_NOTE_ACTIVE(n, false);
              removeNoteFromArpeggiatorMode(n);
            }
          }
          isTriggeringNew = false;
        } else {
          isTriggeringNew = true;
          // Normaler Modus (momentary): Speichern für Release
          activeSwitchNumNotes[i] = numNotesToPlay;
          for (int n = 0; n < numNotesToPlay; n++) {
            activeSwitchNotes[i][n] = notesToPlay[n];
          }
        }
      }
      
      // Play notes
      for (int noteIdx = 0; noteIdx < numNotesToPlay; noteIdx++) {
        int noteToPlay = notesToPlay[noteIdx];
        if (holdMode) {
          if (additiveMode) {
            if (isTriggeringNew) {
              // Additive Hold: Turning switch ON
              if (!IS_HOLD_NOTE_ACTIVE(noteToPlay)) {
                SET_HOLD_NOTE_ACTIVE(noteToPlay, true);
                if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x45);
              }
              if (holdModeNoteRefCount[noteToPlay] < 255) {
                holdModeNoteRefCount[noteToPlay]++;
              }
              addNoteToArpeggiatorMode(noteToPlay);
            } else {
              // Additive Hold: Turning switch OFF
              if (holdModeNoteRefCount[noteToPlay] > 0) {
                holdModeNoteRefCount[noteToPlay]--;
                if (holdModeNoteRefCount[noteToPlay] == 0) {
                  SET_HOLD_NOTE_ACTIVE(noteToPlay, false);
                  if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x00);
                }
              }
              removeNoteFromArpeggiatorMode(noteToPlay);
            }
          } else {
            // Single Hold: Note aktivieren (Alte wurden oben bereits deaktiviert)
            if (isTriggeringNew) {
              SET_HOLD_NOTE_ACTIVE(noteToPlay, true);
              if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x45);
              // RefCounts wurden oben beim loeschen der alten Noten bereits zurueckgesetzt
              holdModeNoteRefCount[noteToPlay] = 1;
              addNoteToArpeggiatorMode(noteToPlay);
            } else {
              // Single Hold Ausschalten (Gleiche Taste nochmal)
              SET_HOLD_NOTE_ACTIVE(noteToPlay, false);
              if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x00);
              holdModeNoteRefCount[noteToPlay] = 0;
              removeNoteFromArpeggiatorMode(noteToPlay);
            }
          }
        } else {
          // Kein Hold Mode: Noten direkt an Arpeggiator geben
          // Falls Arp aus ist, spielen wir die Note statisch
          if (isTriggeringNew) {
            addNoteToArpeggiatorMode(noteToPlay);
            if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x45);
          } else {
            // Dieser Pfad wird bei momentary triggered normal nicht erreicht,
            // aber zur Sicherheit fuer konsistente Logik:
            removeNoteFromArpeggiatorMode(noteToPlay);
            if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x00);
          }
        }
      }
      
      if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
        chordNotesActive[i] = true;
      }
    }
    
    if (switch_released[i]) {
      if (inSubmenu && (currentSubmenu == 1 || currentSubmenu == 3 || currentSubmenu == 4)) {
        // Beim Oktave-Wechsel oder Arp-Menue muessen wir auch die korrekt gespeicherten Noten stoppen
        int numNotesToRelease = activeSwitchNumNotes[i];
        for (int n = 0; n < numNotesToRelease; n++) {
          sendMidiNote(0x90, activeSwitchNotes[i][n], 0x00);
        }
        activeSwitchNumNotes[i] = 0;
      } else {
        int notesToRelease[5];
        int numNotesToRelease = activeSwitchNumNotes[i]; // Nutze gespeicherte Noten
        
        for (int n = 0; n < numNotesToRelease; n++) {
          notesToRelease[n] = activeSwitchNotes[i][n];
        }
        activeSwitchNumNotes[i] = 0; // Speicher leeren
        
        for (int noteIdx = 0; noteIdx < numNotesToRelease; noteIdx++) {
          int noteToRelease = notesToRelease[noteIdx];
          // Wir entfernen die Note IMMER aus dem Arpeggiator (sofern kein Hold aktiv ist)
          // Die Arp-Logik selbst enthaelt den RefCount
          if (!holdMode) {
            removeNoteFromArpeggiatorMode(noteToRelease);
            if (!arpeggiatorActive) sendMidiNote(0x90, noteToRelease, 0x00);
          }
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

