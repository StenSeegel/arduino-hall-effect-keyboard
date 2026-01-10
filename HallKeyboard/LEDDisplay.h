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

bool switchHasActiveNote[13];

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
    uint8_t bgColor = COLOR_BLUE_IDX; // Default
    if (currentSubmenu == 2) bgColor = COLOR_YELLOW_IDX;
    else if (currentSubmenu == 3) bgColor = COLOR_CYAN_IDX;
    else if (currentSubmenu == 4) bgColor = COLOR_WHITE_IDX;

    // LED 0-7 zeigt submenuIndex an
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i == submenuIndex) {
        setLEDColor(i, COLOR_MAGENTA_IDX, 255); // Magenta für Auswahl
      } else if (i < maxSubmenuIndex) {
        setLEDColor(i, bgColor, 50);           // Korrespondierende Farbe für verfügbare Optionen
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
    for (int i = 0; i < 13; i++) {
      int ledIndex = ledMapping[i];
      if (ledIndex < 0) continue;
      
      int midiNote = midiNotes[i] + (currentOctave * 12);
      
      // Check if this note is currently playing
      bool isCurrentlyPlaying = activeMidiNotes[midiNote];
      
      // Check if this note is registered in Arpeggiator sequence
      bool isArpRegistered = false;
      if (arpeggiatorActive) {
        for (int j = 0; j < numHeldArpeggiatorNotes; j++) {
          if (heldArpeggiatorNotes[j] == midiNote) {
            isArpRegistered = true;
            break;
          }
        }
      }

      if (isCurrentlyPlaying || isArpRegistered) {
        switchHasActiveNote[i] = true;
        
        // Determine note color based on current mode
        uint8_t colorIdx;
        uint8_t brightness = 255;

        if (arpeggiatorActive) {
          colorIdx = COLOR_CYAN_IDX;
          if (isArpRegistered && !isCurrentlyPlaying) {
            brightness = 40; // Schwach leuchten für registrierte Noten
          }
        } else if (chordModeActive && chordModeType != 0) {
          colorIdx = COLOR_YELLOW_IDX;
        } else if (playModeActive) {
          colorIdx = COLOR_GREEN_IDX;
        } else {
          colorIdx = COLOR_WHITE_IDX;
        }
        
        setLEDColor(ledIndex, colorIdx, brightness);
      } else {
        if (switchHasActiveNote[i]) {
          switchHasActiveNote[i] = false;
          // Nur ausschalten wenn kein anderer Switch auf dieses LED gemappt ist und dort eine Note spielt oder registriert ist
          bool otherActive = false;
          for (int j = 0; j < 13; j++) {
            if (i != j && ledMapping[j] == ledIndex) {
               int otherNote = midiNotes[j] + (currentOctave * 12);
               if (activeMidiNotes[otherNote]) otherActive = true;
               
               // Arp check for other mapped keys
               if (arpeggiatorActive) {
                 for (int k = 0; k < numHeldArpeggiatorNotes; k++) {
                   if (heldArpeggiatorNotes[k] == otherNote) otherActive = true;
                 }
               }
            }
          }
          if (!otherActive) turnOffLED(ledIndex);
        }
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
      // Clear all note trackers
      for (int i = 0; i < 13; i++) {
        switchHasActiveNote[i] = false;
      }
    }
    
    // Show Controller LEDs (0-2 for FS1-FS3)
    // LED 0 (FS1): Play Mode
    if (playModeActive) {
      setLEDColor(0, COLOR_BLUE_IDX, 200);
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
      setLEDColor(5, COLOR_CYAN_IDX, 200);
    } else {
      turnOffLED(5);
    }
    
    // LED 6: Off
    turnOffLED(6);
    
    // LED 7 (FS4/Tap Tempo): Handled by updateTapTempoLED()
  }
}

#endif
