// Hall-Effect Keyboard - 13 Switches
// Build upon MIDI Note Player: https://docs.arduino.cc/built-in-examples/communication/Midi/
// Build for Arduino Leonardo: https://content.arduino.cc/assets/Pinout-Leonardo_latest.pdf
//
//

#include "button.h"
#include "arduino_stubs.h"

// ============================================
// INCLUDE HARDWARE CONTROLLER LAYER
// ============================================
#include "HardwareController.h"

// ============================================
// INCLUDE SOFTWARE CONTROLLER LAYER
// ============================================
#include "SoftwareController.h"

// ============================================
// INCLUDE HOLD MODE LAYER
// ============================================
#include "HoldMode.h"

// ============================================
// INCLUDE CHORD MODE LAYER
// ============================================
#include "ChordMode.h"

// ============================================
// INCLUDE ARPEGGIATOR MODE LAYER
// ============================================
#include "ArpeggiatorMode.h"

// ============================================
// INCLUDE MIDI GENERATOR LAYER
// ============================================
#include "MidiGenerator.h"

// ============================================
// INCLUDE LED CONTROLLER LAYER (Physical Control)
// ============================================
#include "LEDController.h"

// ============================================
// INCLUDE LED DISPLAY LAYER (Visual Logic)
// ============================================
#include "LEDDisplay.h"

// ============================================
// INCLUDE LED ANIMATOR LAYER (Visual Effects)
// ============================================
#include "LEDAnimator.h"


void setup() {
  // Initialize Serial
  Serial.begin(115200);
  Serial1.begin(31250);
  
  // Layer 1 Setup: Pins und Hardware
  setupHardwareController();
  
  // Initialize Layers
  initSoftwareController();
  initHoldMode();
  initChordMode();
  initArpeggiatorMode();
  initMidiGenerator();
  
  // Killall MIDI: Sicherstellen, dass keine Noten hängen (Bootup Panic)
  killAllMidiNotes();
  
  initLEDController();
  initLEDDisplay();
  initLEDAnimator();
  
  // Bootup: Visual feedback
  for (int led = 0; led < 8; led++) {
    setLEDColor(led, COLOR_GREEN_IDX, 200);
    syncLEDStrip();
    delay(50);
    turnOffLED(led);
    syncLEDStrip();
  }
  
  // Tap Tempo initialisieren
  tapTempo.setBPM(120);
  tapTempo.setBeatsUntilChainReset(4);
  tapTempo.setMinTaps(3); // Erfordert mindestens 3 Taps (2 Intervalle) um das Tempo zu ändern
  
  Serial.println("System Initialized");
}

void loop() {
  // ============================================
  // HARDWARE CONTROLLER LAYER: Lese Input
  // ============================================
  updateHardwareController();
  
  // ============================================
  // SOFTWARE CONTROLLER LAYER: Update States & Events
  // ============================================
  updateSoftwareController();
  
  // ============================================
  // FUNKTIONS LAYER: Process Notes
  // ============================================
  processNoteSwitches();
  
  // Update Tap Tempo (registriert die Tempo-Taps von FS4)
  // Tempo-Taps nur im Hauptmenü (nicht im Submenü) zulassen!
  if (!inSubmenu) {
    if (functionSwitches[3].trigger()) {
      bpmPriorityBeats = 8;
    }
    tapTempo.update(functionSwitches[3].isDown());
  } else {
    // Im Submenü: Clock weiterlaufen lassen, aber Button-Input ignorieren (false senden)
    tapTempo.update(false);
  }
  
  // Update Arpeggiator Playback (von ArpeggiatorMode.h)
  updateArpeggiatorMode();
  
  // Update MIDI Generator - Koordiniere alle aktiven Modi (von MidiGenerator.h)
  updateMidiGenerator();
  
  // ============================================
  // UPDATE LED LAYERS
  // ============================================
  
  // Layer 2: Intelligent Display Manager
  // Handles ALL states: Notes, Idle, Submenu
  updateLEDDisplay();
  
  // Layer 2: Update Visual Effects (wie animieren)
  updateLEDAnimations();      // Animate Blinks/Pulses
  
  // Layer 3: Sync mit Hardware
  if (ledDirty) {
    syncLEDStrip();           // Sync zu NeoPixel Hardware
  }
}
