/**
 * LED DISPLAY LAYER - INTELLIGENT STATE MANAGER
 * 
 * CORE RESPONSIBILITY: Determine WHAT to display based on system state
 * Handles ALL display modes:
 * 1. Note Playing Mode (show active notes)
 * 2. Idle Mode (show controller status)
 * 3. Submenu Mode (show menu feedback)
 */

#ifndef LED_DISPLAY_H
#define LED_DISPLAY_H

extern bool activeMidiNotes[128];
extern bool playModeActive;
extern bool additiveMode;
extern bool chordModeActive;
extern bool arpeggiatorActive;
extern int arpeggiatorMode;
extern int chordModeType;
extern const int ledMapping[13];
extern const int midiNotes[13];
extern int currentOctave;
extern bool inSubmenu;
extern int currentSubmenu;
extern int submenuIndex;
extern int maxSubmenuIndex;
extern int bpmPriorityBeats;
extern int confirmationSwitchIndex;

extern int numHeldArpeggiatorNotes;
extern int heldArpeggiatorNotes[32];

// Track current display state to avoid redundant updates
enum LEDDisplayState {
  DISPLAY_IDLE,
  DISPLAY_NOTES,
  DISPLAY_SUBMENU
};
LEDDisplayState currentDisplayState = DISPLAY_IDLE;

unsigned long lastNoteActiveTime = 0;
const unsigned long CONTROL_LAYER_DELAY = 500; // Kürzeres Delay für reaktiveres Umschalten

// Multi-Note Blink Tracking
struct MultiNoteInfo {
  int noteIndices[5];    // Bis zu 5 Noten pro LED (fuer 7th+8th Akkorde)
  bool isOutOfOctave[5]; // Tracke ob Note ausserhalb Base-Oktave ist
  bool isActive[5];      // Tracke ob Note aktuell klingt (sounding)
  int count;
};
MultiNoteInfo multiNotesPerLED[8];
unsigned long lastMultiNoteBlink = 0;
const unsigned long MULTI_NOTE_BLINK_INTERVAL = 200;
bool multiNoteBlinkState = false;

void initLEDDisplay() {
}

/**
 * INTELLIGENT LED DISPLAY MANAGER
 * Handles all state transitions automatically
 */
void updateLEDDisplay() {
  int confirmationLedIndex = -1;
  if (confirmationSwitchIndex >= 0) {
    confirmationLedIndex = ledMapping[confirmationSwitchIndex];
  }
  // ============================================
  // STATE 1: SUBMENU MODE
  // ============================================
  if (inSubmenu) {
    if (currentDisplayState != DISPLAY_SUBMENU) {
      currentDisplayState = DISPLAY_SUBMENU;
      turnOffAllLEDs();
    }
    
    // Farb-Mapping für Submenüs basierend auf Function Switch Farben
    uint8_t bgColor = COLOR_RED_IDX; // Default (Play Mode/Hold)
    if (currentSubmenu == 2) bgColor = COLOR_YELLOW_IDX;
    else if (currentSubmenu == 3) bgColor = COLOR_MAGENTA_IDX;
    else if (currentSubmenu == 4) bgColor = COLOR_WHITE_IDX;

    // Unterseiten (Page 1, 2) haben unterschiedliche Helligkeiten und Auswahlfarben
    uint8_t bgBrightness = 50;
    if (currentSubmenuPage == 1) bgBrightness = 80;
    else if (currentSubmenuPage == 2) bgBrightness = 110;

    // LED 0-7 zeigt submenuIndex an
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i == confirmationLedIndex) {
        continue; // Bestätigungs-Blinken hat Priorität
      }
      if (i == submenuIndex) {
        uint8_t selectColor = COLOR_WHITE_IDX; // Default White
        
        // Sonderfall: Wenn Basis-Farbe bereits Weiß ist (Octave Menu), nutze Magenta als Kontrast
        if (currentSubmenu == 4) {
          selectColor = COLOR_MAGENTA_IDX;
        }
        
        setLEDColor(i, selectColor, 255); 
      } else if (i < maxSubmenuIndex) {
        setLEDColor(i, bgColor, bgBrightness);           // Korrespondierende Farbe für verfügbare Optionen
      } else {
        turnOffLED(i);
      }
    }
    return;
  }
  
  // ============================================
  // ============================================
  // CHECK: Are there active notes or performance state?
  // ============================================
  bool performanceActive = false;
  
  // 1. Check currently sounding MIDI notes
  for (int i = 0; i < 128; i++) {
    if (activeMidiNotes[i]) {
      performanceActive = true;
      break;
    }
  }
  
  // 2. Check Arpeggiator memory (stay in note view even during duty cycle silence)
  if (arpeggiatorActive && numHeldArpeggiatorNotes > 0) {
    performanceActive = true;
  }
  
  if (performanceActive) {
    lastNoteActiveTime = millis();
  }
  
  // Decide if we should still be in "Performance/Notes" view based on inactivity timer
  bool stayingInNoteView = performanceActive || (millis() - lastNoteActiveTime < CONTROL_LAYER_DELAY);

  // ============================================
  // STATE 2: PERFORMANCE MODE (Notes)
  // Priority check: BPM Change has precedence for 8 beats
  // ============================================
  if (stayingInNoteView && bpmPriorityBeats <= 0) {
    if (isIdle) isIdle = false;
    
    if (currentDisplayState != DISPLAY_NOTES) {
      currentDisplayState = DISPLAY_NOTES;
      turnOffAllLEDs();
    }
    
    // Show active notes on LEDs 0-7
    int keyboardMin = currentOctave * 12;
    int keyboardMax = keyboardMin + 12;

    // Reset multi-note tracking
    for (int led = 0; led < NUM_LEDS; led++) {
      multiNotesPerLED[led].count = 0;
    }

    for (int led = 0; led < NUM_LEDS; led++) {
      if (led == confirmationLedIndex) {
        continue; // Bestätigungs-Blinken hat Priorität
      }
      bool noteInOctavePlaying = false;
      bool noteInOctaveArp = false;
      bool noteOutOctavePlaying = false;
      bool noteOutOctaveArp = false;

      // 1. Check current physical keyboard range (Notes 0-12 relative to current octave)
      for (int i = 0; i < 13; i++) {
        if (ledMapping[i] != led) continue;

        int midiNoteOnKeyboard = midiNotes[i] + keyboardMin;

        if (activeMidiNotes[midiNoteOnKeyboard]) {
          noteInOctavePlaying = true;
          if (multiNotesPerLED[led].count < 5) {
            int idx = multiNotesPerLED[led].count;
            multiNotesPerLED[led].noteIndices[idx] = i;
            multiNotesPerLED[led].isOutOfOctave[idx] = false;
            multiNotesPerLED[led].isActive[idx] = true;
            multiNotesPerLED[led].count++;
          }
        }
        if (arpeggiatorActive) {
          for (int k = 0; k < numHeldArpeggiatorNotes; k++) {
            if (heldArpeggiatorNotes[k] == midiNoteOnKeyboard) {
              noteInOctaveArp = true;
              // Track fuer Blink, aber nur wenn nicht bereits als Playing getracked
              bool alreadyTracked = false;
              for (int t = 0; t < multiNotesPerLED[led].count; t++) {
                if (multiNotesPerLED[led].noteIndices[t] == i && multiNotesPerLED[led].isOutOfOctave[t] == false) {
                  alreadyTracked = true;
                  break;
                }
              }
              if (!alreadyTracked && multiNotesPerLED[led].count < 5) {
                int idx = multiNotesPerLED[led].count;
                multiNotesPerLED[led].noteIndices[idx] = i;
                multiNotesPerLED[led].isOutOfOctave[idx] = false;
                multiNotesPerLED[led].isActive[idx] = false; // Note ist im Arp-Pool but not playing (otherwise caught above)
                multiNotesPerLED[led].count++;
              }
            }
          }
        }
      }

      // 2. Check ALL OTHER MIDI notes (Folded/Out-Of-Range logic)
      // Prüfe IMMER alle Oktaven, um Multi-Note-Blink zu unterstützen
      for (int note = 0; note < 128; note++) {
        if (note >= keyboardMin && note <= keyboardMax) continue; // Skip physical range
        
        bool isNoteActive = activeMidiNotes[note];
        bool isNoteInArp = false;
        if (arpeggiatorActive) {
          for (int k = 0; k < numHeldArpeggiatorNotes; k++) {
            if (heldArpeggiatorNotes[k] == note) {
              isNoteInArp = true;
              break;
            }
          }
        }

        if (isNoteActive || isNoteInArp) {
          // Does this out-of-range pitch share a pitch class with any key mapped to THIS led?
          int pitchClass = note % 12;
          for (int i = 0; i < 13; i++) {
            if (ledMapping[i] == led && (midiNotes[i] % 12) == pitchClass) {
              if (isNoteActive) noteOutOctavePlaying = true;
              if (isNoteInArp) noteOutOctaveArp = true;
              
              // Track fuer Multi-Note-Blink: Erlaube gleichen Index, wenn Oktavstatus unterschiedlich ist
              bool alreadyTracked = false;
              int trackedIdx = -1;
              for (int t = 0; t < multiNotesPerLED[led].count; t++) {
                if (multiNotesPerLED[led].noteIndices[t] == i && multiNotesPerLED[led].isOutOfOctave[t] == true) {
                  alreadyTracked = true;
                  trackedIdx = t;
                  break;
                }
              }
              if (!alreadyTracked && multiNotesPerLED[led].count < 5) {
                int idx = multiNotesPerLED[led].count;
                multiNotesPerLED[led].noteIndices[idx] = i;
                multiNotesPerLED[led].isOutOfOctave[idx] = true;
                multiNotesPerLED[led].isActive[idx] = isNoteActive;
                multiNotesPerLED[led].count++;
              } else if (alreadyTracked && isNoteActive) {
                // Wenn bereits getracked but previously inactive, update to active
                multiNotesPerLED[led].isActive[trackedIdx] = true;
              }
            }
          }
        }
      }

      if (noteInOctavePlaying || noteInOctaveArp || noteOutOctavePlaying || noteOutOctaveArp) {
        // Standardfarbe: Weiss fuer normale Noten
        uint8_t colorIdx = COLOR_WHITE_IDX;
        uint8_t brightness = 255;

        // OUT-OF-OCTAVE DETECTION: Noten ausserhalb der Base-Oktave erhalten Orange
        bool hasOutOfOctaveNote = (noteOutOctavePlaying || noteOutOctaveArp);
        
        // MULTI-NOTE LOGIC (User Request: Static colors, no blinking)
        if (multiNotesPerLED[led].count >= 2) {
          int soundingBaseCount = 0;
          int soundingOutCount = 0;
          for (int s = 0; s < multiNotesPerLED[led].count; s++) {
            if (multiNotesPerLED[led].isActive[s]) {
              if (multiNotesPerLED[led].isOutOfOctave[s]) soundingOutCount++;
              else soundingBaseCount++;
            }
          }

          if (soundingOutCount > 0) {
            // Wenn 8th Note spielt: Bright Orange
            colorIdx = COLOR_ORANGE_IDX;
            brightness = 255;
          } else if (soundingBaseCount > 0) {
            // Wenn Chord Root Note spielt: Bright White
            colorIdx = COLOR_WHITE_IDX;
            brightness = 255;
          } else {
            // Wenn beide im Pool liegen but not sounding: Dimmed White (static)
            colorIdx = COLOR_WHITE_IDX;
            brightness = 40;
          }
        } else {
          // Single note display
          // HIGH PRIORITY: In Octave matches
          if (noteInOctavePlaying || noteInOctaveArp) {
            // Aktuell klingende Noten: Volle Helligkeit
            if (noteInOctavePlaying) {
              brightness = 255;
            } else if (noteInOctaveArp && !noteInOctavePlaying) {
              // Im Arpeggiator registrierte, aber gerade nicht klingende Noten: Gedimmt
              brightness = 40;
            }
          } else {
            // LOW PRIORITY: Out of Octave matches
            colorIdx = COLOR_ORANGE_IDX;
            if (noteOutOctavePlaying) {
              // Out-of-Range Noten die aktuell klingen: Volle Helligkeit
              brightness = 255;
            } else if (noteOutOctaveArp && !noteOutOctavePlaying) {
              // Out-of-Range Noten im Arpeggiator registriert, aber nicht klingend: Stark gedimmt
              brightness = 40;
            }
          }
        }
        
        setLEDColor(led, colorIdx, brightness);
      } else {
        turnOffLED(led);
      }
    }
  }
  // ============================================
  // STATE 3: IDLE MODE - Show Controller Status
  // ============================================
  else {
    // Wenn keine Noten mehr spielen, gehe zurück in den Idle/Hauptmenü-Status
    if (!isIdle) {
      isIdle = true;
    }

    if (currentDisplayState != DISPLAY_IDLE) {
      currentDisplayState = DISPLAY_IDLE;
      turnOffAllLEDs();
    }
    
    // Show Controller LEDs (0-2 for FS1-FS3)
    // LED 0 (FS1): Hold Mode (Rot = Hold, Orange = Additive Hold)
    if (playModeActive) {
      if (confirmationLedIndex == 0) {
        // Bestätigungs-Blinken hat Priorität
      } else {
      uint8_t holdColor = additiveMode ? COLOR_ORANGE_IDX : COLOR_RED_IDX;
      setLEDColor(0, holdColor, 200);
      }
    } else {
      if (confirmationLedIndex != 0) {
        turnOffLED(0);
      }
    }
    
    // LED 1: Off (Previously Chord Mode)
    if (confirmationLedIndex != 1) turnOffLED(1);
    
    // LED 2 (FS2): Chord Mode
    if (chordModeActive && chordModeType != 0) {
      if (confirmationLedIndex != 2) {
      setLEDColor(2, COLOR_YELLOW_IDX, 200);
      }
    } else {
      if (confirmationLedIndex != 2) {
        turnOffLED(2);
      }
    }
    
    // LED 3-4: Off
    if (confirmationLedIndex != 3) turnOffLED(3);
    if (confirmationLedIndex != 4) turnOffLED(4);

    // LED 5 (FS3): Arpeggiator Mode
    if (arpeggiatorActive) {
      if (confirmationLedIndex != 5) {
        setLEDColor(5, COLOR_MAGENTA_IDX, 200);
      }
    } else {
      if (confirmationLedIndex != 5) {
        turnOffLED(5);
      }
    }
    
    // LED 6: Off
    if (confirmationLedIndex != 6) turnOffLED(6);
    
    // LED 7 (FS4/Tap Tempo): Handled by updateTapTempoLED()
  }
}

#endif
