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
int currentSubmenuPage = 0;
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
extern int arpeggiatorDutyCycle;
bool autoHoldActivatedByArp = false;
int savedPlayModeTypeBeforeArp = 0;
bool savedPlayModeActiveBeforeArp = false;

// Forward Declarations für Arpeggiator Synchronisation
extern void resetArpeggiatorPhase();
extern bool arpWaitingForSync;

// New: Reference counting for overlapping chords in Additive Hold
uint8_t holdModeNoteRefCount[128];

// Save/Restore Variables
int savedArpeggiatorModeBeforeSubmenu = 0;
int savedArpeggiatorRateBeforeSubmenu = 2; // Default RATE_EIGHTH
int savedArpeggiatorDutyCycleBeforeSubmenu = 50;
int savedOctaveBeforeSubmenu = 3;
bool savedPlayModeActiveBeforeSubmenu = false;
bool savedChordModeActiveBeforeSubmenu = false;
bool savedArpeggiatorActiveBeforeSubmenu = false;

// ============================================
// EXTERN GLOBALS (from HallKeyboard.ino / other headers)
// ============================================
extern int currentOctave;
extern ArduinoTapTempo tapTempo;
extern bool midiClockActive;
extern uint16_t calculatedBPM;
extern void syncMidiClockPhase();
extern void syncMidiClockToBPM();
extern int bpmPriorityBeats;
extern void sendMidiNote(int cmd, int pitch, int velocity);
extern void setLED(int switchIndex, bool on, bool skipLEDs = false);
extern void confirmLED(int switchIndex);
extern void showOctaveLED(int octave);
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

// Toggle Play Mode Ein/Aus
void togglePlayModeOnOff() {
  if (arpeggiatorActive) {
    // Im Arp Modus: IMMER aktiv, Toggled zwischen hold / latch und hold additive!
    playModeType = (playModeType == PLAY_MODE_HOLD) ? PLAY_MODE_ADDITIVE : PLAY_MODE_HOLD;
    playModeActive = true;
    holdMode = true;
    additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
    
    autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik
    return;
  }

  playModeActive = !playModeActive;
  autoHoldActivatedByArp = false; // Manuelle Änderung überschreibt Automatik
  
  if (!playModeActive) {
    // Wenn Play Mode aus, alle gehaltenen Noten sofort beenden
    for (int i = 0; i < 128; i++) {
      holdModeNoteRefCount[i] = 0;
      if (holdModeMidiNotes[i]) {
        sendMidiNote(0x90, i, 0x00);
        holdModeMidiNotes[i] = false;
        // BUG FIX: Wenn Hold deaktiviert wird, Noten auch aus Arp entfernen (falls sie nicht physikalisch gehalten werden)
        removeNoteFromArpeggiatorMode(i); 
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
  
  //Serial.print("Play Mode: ");
  //Serial.println(playModeActive ? "EIN" : "AUS");
  
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
       if (holdModeMidiNotes[i]) {
         sendMidiNote(0x90, i, 0x00);
         holdModeMidiNotes[i] = false;
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
        if (holdModeMidiNotes[i]) {
          sendMidiNote(0x90, i, 0x00);
        }
      }
    }
    
    // 2. Bestehende gehaltene Noten (aus manuellem Hold) in den Arpeggiator übertragen
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (heldNotes[i] || switch_held[i]) {
            // Berechne Noten (inkl. Chords)
            int baseNote = midiNotes[i] + (currentOctave * 12);
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
      savedPlayModeTypeBeforeArp = playModeType;
      
      playModeActive = true;
      // Bei Sequence Mode erzwingen wir Additive, sonst Latch (Hold)
      if (arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
        playModeType = PLAY_MODE_ADDITIVE;
      }
      
      holdMode = true;
      additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
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
      // Auch alle gehaltenen Noten stoppen und RefCounts zurücksetzen
      for (int i = 0; i < 128; i++) {
        holdModeNoteRefCount[i] = 0;
        if (holdModeMidiNotes[i]) {
          sendMidiNote(0x90, i, 0x00);
          holdModeMidiNotes[i] = false;
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
      maxSubmenuIndex = 2;
      submenuIndex = playModeType;
      break;
    case 2:
      if (page == 0) {
        maxSubmenuIndex = NUM_SCALE_TYPES;
        submenuIndex = scaleType;
      } else {
        maxSubmenuIndex = 2; // Extended, Folded
        submenuIndex = (chordModeType == CHORD_MODE_FOLDED) ? 1 : 0;
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
            holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
            additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
          }
        }
        break;
      case 2:
        if (currentSubmenuPage == 0) {
          if (submenuIndex != scaleType) {
            scaleType = submenuIndex;
          }
        } else {
          chordModeType = (submenuIndex == 1) ? CHORD_MODE_FOLDED : CHORD_MODE_EXTENDED;
        }
        break;
      case 3:
        if (currentSubmenuPage == 0) {
          if (submenuIndex != arpeggiatorMode) {
            int oldArpMode = arpeggiatorMode;
            arpeggiatorMode = submenuIndex;
            
            // Sequence Mode Sonderfall: Aktiviere Additive (Hold ist eh an)
            if (arpeggiatorActive && arpeggiatorMode == ARPEGGIATOR_SEQUENCE) {
                playModeType = PLAY_MODE_ADDITIVE;
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
      case 4:
        showOctaveLED(currentOctave);
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
          extern uint16_t masterPulseCounter;
          extern uint16_t ppqnCounter;
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
            if (currentSubmenu == 2) numPages = 2;
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
              if (holdModeMidiNotes[n]) {
                holdModeMidiNotes[n] = false;
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
              holdModeNoteRefCount[n] = 0;
              if (holdModeMidiNotes[n]) {
                if (!arpeggiatorActive) sendMidiNote(0x90, n, 0x00);
                holdModeMidiNotes[n] = false;
                removeNoteFromArpeggiatorMode(n);
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
            holdModeNoteRefCount[n] = 0;
            if (holdModeMidiNotes[n]) {
              if (!arpeggiatorActive) sendMidiNote(0x90, n, 0x00);
              holdModeMidiNotes[n] = false;
              removeNoteFromArpeggiatorMode(n);
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
        if (holdMode) {
          if (additiveMode) {
            if (isTriggeringNew) {
              // Additive Hold: Turning switch ON
              if (!holdModeMidiNotes[noteToPlay]) {
                holdModeMidiNotes[noteToPlay] = true;
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
                  holdModeMidiNotes[noteToPlay] = false;
                  if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x00);
                }
              }
              removeNoteFromArpeggiatorMode(noteToPlay);
            }
          } else {
            // Single Hold: Note aktivieren (Alte wurden oben bereits deaktiviert)
            if (isTriggeringNew) {
              holdModeMidiNotes[noteToPlay] = true;
              if (!arpeggiatorActive) sendMidiNote(0x90, noteToPlay, 0x45);
              // RefCounts wurden oben beim loeschen der alten Noten bereits zurueckgesetzt
              holdModeNoteRefCount[noteToPlay] = 1;
              addNoteToArpeggiatorMode(noteToPlay);
            } else {
              // Single Hold Ausschalten (Gleiche Taste nochmal)
              holdModeMidiNotes[noteToPlay] = false;
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

