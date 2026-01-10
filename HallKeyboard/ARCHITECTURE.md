# Hall-Effect Keyboard - Technical Architecture

## Overview

The Hall-Effect Keyboard firmware implements a **modular 5-layer mode system** with a dedicated **3-layer LED display system**, all optimized for Arduino Leonardo (2560 bytes RAM, 28KB flash).

---

## Architecture Layers

### LAYER 1: Hardware Controller
**File**: `HardwareController.h`

Handles raw input from physical hardware:
- 13 Hall-effect switches (chromatically arranged C-C)
- 4 Function switches (FS1-FS4)
- Debouncing and state management via `button.h`

**Key Functions**:
- `setupHardwareController()` - Initialize pins and buttons
- `updateHardwareController()` - Poll all sensors
- `getHardwareMIDINote()` - Calculate MIDI pitch including octave

---

### LAYER 2: Software Controller
**File**: `SoftwareController.h`

Central dispatcher managing mode state and submenu navigation:
- Mode switching (Play, Chord, Arpeggiator, Tap Tempo)
- Submenu system (4 menus for Octave, Root, Sync, Arp)
- State management and coordination

**Key Variables**:
- `playModeActive`, `chordModeActive`, `arpeggiatorActive`
- `inSubmenu`, `currentSubmenu`, `submenuIndex`
- `currentOctave`, `isIdle`

**Key Functions**:
- `processNoteSwitches()` - Main note event processor
- `updateSoftwareController()` - Handle function switches and timers
- `enterSubmenu()` / `exitSubmenu()` - Submenu navigation

---

### LAYERS 3-6: Independent Mode Layers

#### 3. Hold Mode
**File**: `HoldMode.h`

Sustains notes until toggled off:
- Normal mode: One note held (replaces previous)
- Additive mode: Unlimited simultaneous held notes

**State**: `holdModeMidiNotes[128]`

**Key Functions**:
- `updateHoldMode()` - Process note events for sustaining logic
- `initHoldMode()` - Reset state

---

#### 4. Chord Mode
**File**: `ChordMode.h`

Generates complex chords from MIDI input:
- 9 Scale Types (Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Locrian, Power 5/8)
- 7 Diatonic positions per scale
- Extended & Folded voicings

**Key Functions**:
- `turnOnChordNotesImpl()` / `turnOffChordNotesImpl()` - LED and Note control
- `getChordNote()` - Calculate semitone offsets
- `clearChordMode()` - Cleanup

---

#### 5. Arpeggiator Mode
**File**: `ArpeggiatorMode.h`

Real-time note sequencing engine:
- 4 Patterns (Up, Down, Up-Down, Down-Up)
- 5 Rhythmic rates (1/4 to dotted 1/8)
- Tap Tempo sync via `ArduinoTapTempo`

**Key Functions**:
- `updateArpeggiatorMode()` - Timing and sequence engine
- `playNextArpeggiatorNote()` - Step logic
- `addNoteToArpeggiatorMode()` / `removeNoteFromArpeggiatorMode()` - Dynamic sequence tracking

---

#### 6. MIDI Generator
**File**: `MidiGenerator.h`

The physical output layer:
- Dispatches MIDI commands via `Serial1`
- Tracks all active notes for LED feedback
- Coordinates concurrent note sources (Hold, Chord, Arp)

**Key Functions**:
- `updateMidiGenerator()` - Sync loop
- `sendMidiNote()` - Low-level MIDI command dispatcher
- `initMidiGenerator()` - State cleanup

---

## LED Display System

### 3-Layer Modular Architecture

#### Layer 1: LEDController
**File**: `LEDController.h`

**Purpose**: Physical hardware layer - maps logic to WS2812 hardware

**Architecture**:
- Stores `Adafruit_NeoPixel` instance
- Abstracted LED mapping for different hardware layouts
- Sync methods with dirty-flag optimization

**Key Functions**:
- `initLEDController()` - Setup hardware pins
- `setLEDForSwitch()` - Physical LED setter
- `syncLEDStrip()` - Trigger hardware refresh

---

#### Layer 2: LEDDisplay
**File**: `LEDDisplay.h`

**Purpose**: State logic layer - determines WHAT is shown

**Features**:
- `idle` mode feedback (show current feature set)
- `submenu` mode feedback (menu selection)
- Note visualization (MIDI note tracking)

**Key Functions**:
- `updateLEDDisplay()` - Main visual state coordinator
- `showMenuFeedback()` - Visual selection guide

---

#### Layer 3: LEDAnimator
**File**: `LEDAnimator.h`

**Purpose**: Effects layer - manages animation and timing

**Minimal State Design**:
- Non-blocking blink/pulse logic
- Multi-note conflict resolution (blinking shared LEDs)
- Tap Tempo "Heartbeat" pulse

**Key Functions**:
- `updateLEDAnimations()` - Main effect updater
- `confirmLEDImpl()` - Trigger confirmation blink
- `updateTapTempoLED()` - Status pulse logic

---

## File Structure

```
HallKeyboard/
├── HallKeyboard.ino           (Main glue logic)
├── HardwareController.h       (Sensor reading)
├── SoftwareController.h       (Navigation & Logic)
├── HoldMode.h                 (Sustain features)
├── ChordMode.h                (Harmonic features)
├── ArpeggiatorMode.h          (Rhythmic features)
├── MidiGenerator.h            (MIDI stack)
├── LEDController.h            (LED driver)
├── LEDDisplay.h               (Visual state)
├── LEDAnimator.h              (Visual effects)
├── button.h                   (Debouncer)
├── ArduinoTapTempo.h          (Timing lib)
└── ARCHITECTURE.md            (This file)
```

**Last Updated**: 10. Januar 2026
**Version**: 2.1.0-Modular
