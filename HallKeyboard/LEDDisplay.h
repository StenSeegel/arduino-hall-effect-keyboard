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

extern int numHeldArpeggiatorNotes;
extern int heldArpeggiatorNotes[13];

// Track current display state to avoid redundant updates
enum LEDDisplayState {
  DISPLAY_IDLE,
  DISPLAY_NOTES,
  DISPLAY_SUBMENU
};
LEDDisplayState currentDisplayState = DISPLAY_IDLE;

unsigned long lastNoteActiveTime = 0;
const unsigned long CONTROL_LAYER_DELAY = 500; // Kürzeres Delay für reaktiveres Umschalten

void initLEDDisplay() {
}

/**
 * INTELLIGENT LED DISPLAY MANAGER
 * Handles all state transitions automatically
 */
void updateLEDDisplay() {
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
      if (i == submenuIndex) {
        uint8_t selectColor = COLOR_MAGENTA_IDX; // Page 0
        if (currentSubmenuPage == 1) selectColor = COLOR_WHITE_IDX;
        else if (currentSubmenuPage == 2) selectColor = COLOR_YELLOW_IDX;
        
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

    for (int led = 0; led < NUM_LEDS; led++) {
      bool noteInOctavePlaying = false;
      bool noteInOctaveArp = false;
      bool noteOutOctavePlaying = false;
      bool noteOutOctaveArp = false;

      // 1. Check current physical keyboard range (Notes 0-12 relative to current octave)
      for (int i = 0; i < 13; i++) {
        if (ledMapping[i] != led) continue;

        int midiNoteOnKeyboard = midiNotes[i] + keyboardMin;

        if (activeMidiNotes[midiNoteOnKeyboard]) noteInOctavePlaying = true;
        if (arpeggiatorActive) {
          for (int k = 0; k < numHeldArpeggiatorNotes; k++) {
            if (heldArpeggiatorNotes[k] == midiNoteOnKeyboard) noteInOctaveArp = true;
          }
        }
      }

      // 2. Check ALL OTHER MIDI notes (Folded/Out-Of-Range logic)
      // Only check if we don't already have an "In-Octave" note for this LED
      // mapping out-of-range pitches to their closest pitch class equivalents on the keys.
      if (!noteInOctavePlaying && !noteInOctaveArp) {
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
              }
            }
          }
        }
      }

      if (noteInOctavePlaying || noteInOctaveArp || noteOutOctavePlaying || noteOutOctaveArp) {
        // Determine note color based on current mode
        uint8_t colorIdx;
        uint8_t brightness = 255;

        // HIGH PRIORITY: In Octave matches
        if (noteInOctavePlaying || noteInOctaveArp) {
          if (arpeggiatorActive) {
            colorIdx = COLOR_MAGENTA_IDX; // Arp = Magenta
            if (noteInOctaveArp && !noteInOctavePlaying) {
              brightness = 40; // Schwach leuchten für registrierte Noten
            }
          } else if (chordModeActive && chordModeType != 0) {
            colorIdx = COLOR_YELLOW_IDX;
          } else if (playModeActive) {
            colorIdx = COLOR_RED_IDX;
          } else {
            colorIdx = COLOR_WHITE_IDX;
          }
        } else {
          // LOW PRIORITY: Out of Octave matches
          if (arpeggiatorActive && (noteOutOctavePlaying || noteOutOctaveArp)) {
            colorIdx = COLOR_ORANGE_IDX; // Arp OOR = Orange
            if (noteOutOctaveArp && !noteOutOctavePlaying) {
              brightness = 40; // Dim for registered notes
            } else {
              brightness = 255; // Full brightness for playing notes (Blink effect)
            }
          } else if (chordModeActive) {
            colorIdx = COLOR_ORANGE_IDX; // Chord OOR = Orange
            brightness = 150;
          } else {
            colorIdx = COLOR_CYAN_IDX; // General OOR = Cyan
            brightness = 150;
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
    // LED 0 (FS1): Play Mode
    if (playModeActive) {
      setLEDColor(0, COLOR_RED_IDX, 200);
    } else {
      turnOffLED(0);
    }
    
    // LED 1: Off (Previously Chord Mode)
    turnOffLED(1);
    
    // LED 2 (FS2): Chord Mode
    if (chordModeActive && chordModeType != 0) {
      setLEDColor(2, COLOR_YELLOW_IDX, 200);
    } else {
      turnOffLED(2);
    }
    
    // LED 3-4: Off
    turnOffLED(3);
    turnOffLED(4);

    // LED 5 (FS3): Arpeggiator Mode
    if (arpeggiatorActive) {
      setLEDColor(5, COLOR_MAGENTA_IDX, 200);
    } else {
      turnOffLED(5);
    }
    
    // LED 6: Off
    turnOffLED(6);
    
    // LED 7 (FS4/Tap Tempo): Handled by updateTapTempoLED()
  }
}

#endif
