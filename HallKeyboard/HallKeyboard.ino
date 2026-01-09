// Hall-Effect Keyboard - 13 Switches
// Build upon MIDI Note Player: https://docs.arduino.cc/built-in-examples/communication/Midi/
// Build for Arduino Leonardo: https://content.arduino.cc/assets/Pinout-Leonardo_latest.pdf
// 
// Modi:
// - Normal: Eine Note pro Taste, solange die Taste gedrückt ist
// - Hold (A1): Taste gedrückt -> Note hält an, erneut gedrückt -> Note aus
// - Additive (A2): Mehrere Noten gleichzeitig halten // Deaktiviert
// - Akkord (Kombiniert mit A1): Taste triggert vordefinierten Akkord
// - Oktave (A3/A4): Oktave rauf/runter
//

#include "button.h"
#include "arduino_stubs.h"
#include "Adafruit_NeoPixel.h"

const int NUM_SWITCHES = 13;

// WS2812 LED Konfiguration
const int NUM_LEDS = 8;              // 8 WS2812 LEDs
const int LED_PIN = A5;              // Pin A5 als Digital
const int LED_BRIGHTNESS = 55;      // LED Helligkeit (0-255)

// Farben als RGB-Hex, werden mit pixels.Color() korrekt zu GRB konvertiert
const uint32_t WHITE_KEY_COLOR = 0xFFFFFF;   // Weiß für weiße Tasten
const uint32_t BLACK_KEY_COLOR = 0xFF69B4;   // Hot Pink für schwarze Tasten

// Index-Palette Farben für Controller-Modus und Submenu-System
// Reine, vollgesättigte RGB-Farben (mit Rot-Grün Tausch korrigiert)
// Format: 0xRRGGBB - Rot und Grün sind im System getauscht, daher entsprechend angepasst
const uint32_t INDEX_PALETTE[8] = {
  0x00FF00,  // Index 0 - Rot (wird als Rot angezeigt nach Tausch)
  0xFFFF00,  // Index 1 - Gelb (bleibt Gelb)
  0xFF0000,  // Index 2 - Grün (wird als Grün angezeigt nach Tausch)
  0xFF00FF,  // Index 3 - Cyan (wird als Cyan angezeigt nach Tausch)
  0x0000FF,  // Index 4 - Blau (bleibt Blau)
  0x00FFFF,  // Index 5 - Magenta (wird als Magenta angezeigt nach Tausch)
  0xFFFFFF,  // Index 6 - Weiß (bleibt Weiß)
  0x7FFF00   // Index 7 - Orange (Rot+Grün, angepasst für Tausch)
};

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Pin-Belegung für alle 13 Switches
const int switchPins[NUM_SWITCHES] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  // Digital Pins 2-12 (ohne 13)
  18                                 // D17 (A3), D18 (A4) als Digital
};

// MIDI Noten für jeden Switch (C bis C, relativ zu Oktave 0)
const int midiNotes[NUM_SWITCHES] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12  // C, C#, D, D#, E, F, F#, G, G#, A, A#, B, C
};

// LED-Mapping: Switch-Index -> LED-Index (weiße und schwarze Tasten)
// Weiße Tasten: C(0)=LED0, D(2)=LED1, E(4)=LED2, F(5)=LED3, G(7)=LED4, A(9)=LED5, B(11)=LED6, C(12)=LED7
// Schwarze Tasten: C#(1)=LED0, D#(3)=LED1, F#(6)=LED3, G#(8)=LED4, A#(10)=LED5
const int ledMapping[NUM_SWITCHES] = {
  0,  // Switch 0 (C)  -> LED 0
  0,  // Switch 1 (C#) -> LED 0 (schwarze Taste)
  1,  // Switch 2 (D)  -> LED 1
  1,  // Switch 3 (D#) -> LED 1 (schwarze Taste)
  2,  // Switch 4 (E)  -> LED 2
  3,  // Switch 5 (F)  -> LED 3
  3,  // Switch 6 (F#) -> LED 3 (schwarze Taste)
  4,  // Switch 7 (G)  -> LED 4
  4,  // Switch 8 (G#) -> LED 4 (schwarze Taste)
  5,  // Switch 9 (A)  -> LED 5
  5,  // Switch 10 (A#) -> LED 5 (schwarze Taste)
  6,  // Switch 11 (B) -> LED 6
  7   // Switch 12 (C) -> LED 7
};

// Array zum Identifizieren von schwarzen Tasten (C#, D#, F#, G#, A#)
const bool isBlackKey[NUM_SWITCHES] = {
  false, // Switch 0 (C)
  true,  // Switch 1 (C#)
  false, // Switch 2 (D)
  true,  // Switch 3 (D#)
  false, // Switch 4 (E)
  false, // Switch 5 (F)
  true,  // Switch 6 (F#)
  false, // Switch 7 (G)
  true,  // Switch 8 (G#)
  false, // Switch 9 (A)
  true,  // Switch 10 (A#)
  false, // Switch 11 (B)
  false  // Switch 12 (C)
};

// Oktav-Einstellung (Standard: 3, entspricht C3)
int currentOctave = 3;

// Button Objekte für jeden Switch
Button switches[NUM_SWITCHES];

// Flag um FS2+FS3 Kombination zu handhaben
bool fs3CombinationHandled = false;
bool fs2CombinationHandled = false;  // Flag für FS2 um doppelte Verarbeitung zu verhindern

// Hold-Schalter Variables
bool holdMode = false;               // Toggle zwischen Normal und Hold Modus
bool additiveMode = false;          // Additive Hold: mehrere Noten gleichzeitig
int heldNote = -1;                  // Aktuell gehaltene Note (-1 = keine) - nur für nicht-additiv
bool heldNotes[NUM_SWITCHES];       // Array für additive Noten

// Akkord-Modus Variables
int chordModeType = 0;              // 0=off, 1=extended, 2=folded
#define CHORD_MODE_OFF 0
#define CHORD_MODE_EXTENDED 1
#define CHORD_MODE_FOLDED 2
int scaleType = 0;                  // 0-8: Alle diatonischen Modi + Power Chords
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
bool diatonicIsMajor = true;        // wird für Kompatibilität beibehalten

// Diatonische Akkord-Einstellungen
int diatonicRootKey = 0;            // 0-11 entspricht C-B
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

// Akkord-Definitionen: Speichere nur die Akkordstrukturen
const int maxChordNotes = 3;
// Akkordtypen: [Index] = {Semitone offsets}
const int chordDefinitions[7][3] = {
  {0, 4, 7},    // 0 = Major
  {0, 3, 7},    // 1 = Minor
  {0, 7, -1},   // 2 = Power 5 (ohne 3. Note)
  {0, 7, 12},   // 3 = Power 8 (mit Oktave)
  {0, 5, 7},    // 4 = Sus4
  {0, 4, 8},    // 5 = Augmented
  {0, 3, 6}     // 6 = Diminished
};

// Diatonische Akkord-Muster für alle 7 Modi
// Index 0-6 entspricht den Stufen der Tonleiter (I-VII)
// Das Pattern ist basierend auf der Dur-Tonleiter: Major, minor, minor, Major, Major, minor, Diminished
// und wird mit (diatonicDegree + scaleType) % 7 für jeden Modus neu indexiert
const int diatonicChordPattern[7] = {0, 1, 1, 0, 0, 1, 6};  // Major, minor, minor, Major, Major, minor, Dim

// Intervall-Pattern für alle diatonischen Modi
// {2, 2, 1, 2, 2, 2, 1} = Ganz, Ganztöne, Halbtöne (in Semitonen)
// Das Pattern wird mit (stepIndex + scaleType) % 7 rotiert für jeden Modus
const int modeStepIntervals[7] = {2, 2, 1, 2, 2, 2, 1};  // Intervalle für Ionian

// Helper-Funktion um die Semitone für einen Modus zu berechnen
int getModeNote(int degree, int mode) {
  int semitones = 0;
  for (int i = 0; i < degree; i++) {
    semitones += modeStepIntervals[(i + mode) % 7];
  }
  return semitones;
}

// Hilfsfunktion um zu prüfen ob eine Note in der diatonischen Tonleiter ist
bool isDiatonicNote(int switchIndex) {
  int noteOffset = (midiNotes[switchIndex] - diatonicRootKey + 12) % 12;
  
  if (scaleType >= 0 && scaleType <= 6) {
    // Prüfe gegen alle 7 Töne des Modus
    for (int i = 0; i < 7; i++) {
      if (noteOffset == getModeNote(i, scaleType)) {
        return true;
      }
    }
    return false;
  }
  
  // Für Power Modes alle Noten erlauben
  return true;
}

// Hilfsfunktion um diatonischen Akkordtyp zu berechnen
// switchIndex: 0-12 (C bis C)
// Gibt zurück welcher Akkordtyp (0-6) für diese Taste verwendet werden sollte
int getDiatonicChordType(int switchIndex) {
  int noteOffset = (midiNotes[switchIndex] - diatonicRootKey + 12) % 12;
  
  // Bestimme die diatonische Stufe (0-6)
  int diatonicDegree = -1;
  for (int i = 0; i < 7; i++) {
    if (noteOffset == getModeNote(i, scaleType)) {
      diatonicDegree = i;
      break;
    }
  }
  
  if (diatonicDegree == -1) {
    // Nicht-diatonische Note - verwende Major als Default
    return 0;
  }
  
  // Gebe den Akkordtyp basierend auf dem Muster zurück
  if (scaleType >= 0 && scaleType <= 6) {
    int modalIndex = (diatonicDegree + scaleType) % 7;
    return diatonicChordPattern[modalIndex];
  } else {
    // Fallback für nicht-diatonische Modi
    return 0; // Major
  }
}

// Hilfsfunktion um die Akkordnoten zu erhalten
int getChordNote(int switchIndex, int variationType, int noteIndex) {
  int chordDefIndex;
  
  switch(variationType % NUM_SCALE_TYPES) {
    case SCALE_IONIAN:     // Ionian (Major)
    case SCALE_DORIAN:     // Dorian
    case SCALE_PHRYGIAN:   // Phrygian
    case SCALE_LYDIAN:     // Lydian
    case SCALE_MIXOLYDIAN: // Mixolydian
    case SCALE_AEOLIAN:    // Aeolian (Natural Minor)
    case SCALE_LOCRIAN:    // Locrian
      chordDefIndex = getDiatonicChordType(switchIndex);
      break;
    case SCALE_POWER5:     // Power 5
      chordDefIndex = 2;   // Power 5 (0, 7, -1)
      break;
    case SCALE_POWER8:     // Power 8
      chordDefIndex = 3;   // Power 8 (0, 7, 12)
      break;
    default:
      chordDefIndex = 0;   // Major
  }
  
  return chordDefinitions[chordDefIndex][noteIndex];
}

bool chordNotesActive[NUM_SWITCHES];  // Speichert ob Akkord-Noten für einen Switch aktiv sind

// Submenu-System Variablen
bool inSubmenu = false;              // Flag ob wir aktuell in einem Submenu sind
int currentSubmenu = 0;              // Welches Submenu aktiv ist (1-4)
int submenuIndex = 0;                // Aktueller Index im Submenu
int maxSubmenuIndex = 0;             // Max Index für aktuelles Submenu
bool submenuChanged = false;         // Flag ob Submenu-Status geändert wurde
int savedOctaveBeforeSubmenu = 3;    // Speichert die Oktave bevor Submenu 4 geöffnet wird

// Play Mode Variablen (für Submenu 1)
int playModeType = 0;                // 0=Hold, 1=Hold+Additive (Default: Hold)
bool playModeActive = false;         // Flag ob Play Mode aktiv ist
#define PLAY_MODE_HOLD 0
#define PLAY_MODE_ADDITIVE 1

// Chord Mode Variablen
bool chordModeActive = false;        // Flag ob Chord Mode aktiv ist

// LED Modi
bool ledControllerMode = true;       // true=Controller-Modus, false=Noten-Modus
bool isIdle = true;                  // true=Zeige Controller LEDs, false=Zeige nur Noten LEDs

// MIDI Note Tracking - verfolgt welche MIDI-Noten gerade aktiv sind
bool activeMidiNotes[128];           // Array für alle möglichen MIDI-Noten (0-127)

// Timing für LED-Blinken
unsigned long lastBlinkTime = 0;
int blinkCounter = 0;
bool blinkState = false;
int confirmationSwitchIndex = -1;    // Welcher Switch bestätigt werden soll

// Multi-Note Blink Tracking - für LEDs mit mehreren aktiven Noten
int ledMultiNoteCount[NUM_LEDS];     // Anzahl aktiver Noten pro LED
int ledMultiNoteSwitches[NUM_LEDS][2]; // Die zwei aktiven Switch-Indizes (für Farbwechsel)
unsigned long lastLEDBlinkTime = 0;
bool ledBlinkState = false;
const unsigned long LED_BLINK_INTERVAL = 250;  // 250ms zwischen Farbwechsel

// Oktave-LED Anzeige Timing
bool octaveLEDActive = false;
unsigned long octaveLEDStartTime = 0;
const unsigned long OCTAVE_LED_DURATION = 800;  // 0.8 Sekunden anzeigen

// LED-Bestätigungs-Blinken (3x 300ms)
void confirmLED(int switchIndex) {
  confirmationSwitchIndex = switchIndex;
  blinkCounter = 0;
  blinkState = false;
  lastBlinkTime = millis();
}

// Update LED-Bestätigungs-Blinken (wird in loop() aufgerufen)
void updateConfirmationBlink() {
  if (confirmationSwitchIndex >= 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 300) {
      lastBlinkTime = currentTime;
      int ledIndex = ledMapping[confirmationSwitchIndex];
      
      if (blinkState) {
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
        blinkCounter++;
        if (blinkCounter >= 3) {
          confirmationSwitchIndex = -1;
          // Stelle Controller-LEDs (Index-Marker) wieder her, wenn Blinken fertig
          updateControllerLEDs();
          return;
        }
      } else {
        // Nutze die entsprechende Tastenfarbe für Bestätigung
        uint32_t blinkColor = isBlackKey[confirmationSwitchIndex] ? BLACK_KEY_COLOR : WHITE_KEY_COLOR;
        pixels.setPixelColor(ledIndex, blinkColor);
      }
      blinkState = !blinkState;
      pixels.show();
    }
  }
}

// Update LED-Blinken für mehrere Noten auf gleicher LED
void updateLEDMultiNoteBlink() {
  unsigned long currentTime = millis();
  if (currentTime - lastLEDBlinkTime >= LED_BLINK_INTERVAL) {
    lastLEDBlinkTime = currentTime;
    ledBlinkState = !ledBlinkState;
    
    // Aktualisiere alle LEDs mit mehreren Noten
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
      // Zähle aktive Noten für diese LED neu
      int activeCount = 0;
      int activeSwitches[2];
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (ledMapping[i] == ledIndex && (switches[i].isDown() || heldNotes[i])) {
          if (activeCount < 2) {
            activeSwitches[activeCount] = i;
          }
          activeCount++;
        }
      }
      
      // Blinke nur wenn 2 oder mehr Noten aktiv sind
      if (activeCount >= 2) {
        int switchIndex = ledBlinkState ? activeSwitches[0] : activeSwitches[1];
        uint32_t color = isBlackKey[switchIndex] ? BLACK_KEY_COLOR : WHITE_KEY_COLOR;
        pixels.setPixelColor(ledIndex, color);
      }
    }
    pixels.show();
  }
}

// Hilfsfunktion zum Deaktivieren der Controller-LEDs bei Noten-Events
void disableControllerLEDsForNotes() {
  // Wenn wir im Submenu sind, behalte die Submenu-LEDs
  if (inSubmenu) {
    return;
  }
  
  if (isIdle) {
    isIdle = false;
    pixels.clear();
    pixels.show();
  }
}

// Einfache Idle-Status Verwaltung
void updateIdleStatus() {
  // Prüfe Oktave-LED Timeout
  if (octaveLEDActive && millis() - octaveLEDStartTime >= OCTAVE_LED_DURATION) {
    octaveLEDActive = false;
  }
  
  // Wenn wir im Submenu sind, bleibe IMMER idle (zeige Controller-LEDs)
  if (inSubmenu) {
    isIdle = true;
    return;
  }
  
  // Wenn nicht idle, prüfe ob wir zu idle wechseln können
  if (!isIdle) {
    bool hasActivity = false;
    
    // Prüfe MIDI-Noten Aktivität (das ist was wirklich zählt!)
    for (int i = 0; i < 128; i++) {
      if (activeMidiNotes[i]) {
        hasActivity = true;
        break;
      }
    }
    
    // Prüfe auch aktuell gedrückte Switches (für sofortige Reaktion)
    if (!hasActivity) {
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (switches[i].isDown()) {
          hasActivity = true;
          break;
        }
      }
    }
    
    // Prüfe auch Confirmation und Oktave LEDs
    if (confirmationSwitchIndex >= 0 || octaveLEDActive) {
      hasActivity = true;
    }
    
    // Wenn keine Aktivität, zurück zu idle
    if (!hasActivity) {
      isIdle = true;
      updateControllerLEDs();
    }
  }
}

// Controller-Modus LED Anzeige
void updateControllerLEDs() {
  if (!isIdle) return;  // Nur anzeigen wenn idle
  
  pixels.clear();
  
  // Zeige Status LEDs nur wenn wir NICHT im Submenu sind
  if (!inSubmenu) {
    // LED 1: Play Mode Status
    if (playModeActive) {
      pixels.setPixelColor(0, INDEX_PALETTE[playModeType % 8]);
    }
    
    // LED 2: Chord Mode Status  
    if (chordModeActive && chordModeType > 0) {
      pixels.setPixelColor(1, INDEX_PALETTE[scaleType % 8]);
    }
  } else {
    // LED 8: Submenu-Indikator
    pixels.setPixelColor(7, INDEX_PALETTE[currentSubmenu - 1]);
    // Zeige aktuellen Index in Submenu
    if (submenuIndex >= 0 && submenuIndex < 8) {
      pixels.setPixelColor(submenuIndex, INDEX_PALETTE[submenuIndex % 8]);
    }
  }
  
  pixels.show();
}

// Forward-Deklarationen
void handleShortPress(int fsNumber);
void enterSubmenu(int submenuNumber);
void exitSubmenu(bool saveChanges);
void togglePlayMode();
void togglePlayModeOnOff();
void toggleChordModeOnOff();
void setLED(int switchIndex, bool on, bool skipLEDs = false);
void setLEDWithColor(int switchIndex, bool on, uint32_t customColor = 0xFFFFFF, bool skipLEDs = false);
void playChord(int switchIndex, bool on, bool skipLEDs = false);
void playChordFolded(int switchIndex, bool on, bool skipLEDs = false);
void turnOnChordNotes(int switchIndex, bool isFolded, bool skipLEDs = false);
void turnOffChordNotes(int switchIndex, bool isFolded, bool skipLEDs = false);

// Funktions-Schalter (4 Schalter an A1-A4)
const int NUM_FUNCTION_SWITCHES = 4;
const int functionSwitchPins[NUM_FUNCTION_SWITCHES] = {
  A1, A2, A3, A4
};
Button functionSwitches[NUM_FUNCTION_SWITCHES];
unsigned long functionSwitchPressTime[NUM_FUNCTION_SWITCHES];  // Zeit beim Drücken speichern
bool functionSwitchLongPressed[NUM_FUNCTION_SWITCHES];         // Flag für Long-Press Erkennung
const unsigned long LONG_PRESS_DURATION = 1000;  // 1 Sekunde für Long-Press

void setup() {
  // Alle Switch-Pins initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switches[i].begin(switchPins[i]);
    heldNotes[i] = false;
  }
  
  // Multi-Note Blink Tracking initialisieren
  for (int i = 0; i < NUM_LEDS; i++) {
    ledMultiNoteCount[i] = 0;
    ledMultiNoteSwitches[i][0] = -1;
    ledMultiNoteSwitches[i][1] = -1;
  }
  
  // Analog Pins als Digital mit Pullup ZUERST konfigurieren
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  
  // Funktions-Schalter initialisieren (A1-A4 als Digitalpins mit Pullup)
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    functionSwitches[i].begin(functionSwitchPins[i]);
    functionSwitchLongPressed[i] = false;
  }
  
  // Akkord-Mode Array initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    chordNotesActive[i] = false;
  }
  
  // MIDI-Note-Tracking initialisieren
  for (int i = 0; i < 128; i++) {
    activeMidiNotes[i] = false;
  }
  
  // WS2812 LEDs initialisieren
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);  // Helligkeit einstellen
  pixels.clear();
  pixels.show();
  
  // Bootup: Regenbogen-Animation von links nach rechts über alle LEDs mit Fade In/Out
  for (int led = 0; led < NUM_LEDS; led++) {
    // Fade In
    for (int brightness = 0; brightness <= LED_BRIGHTNESS; brightness += 10) {
      pixels.clear();
      pixels.setBrightness(brightness);
      pixels.setPixelColor(led, INDEX_PALETTE[led % 8]);
      pixels.show();
      delay(20);
    }
    
    // Fade Out
    for (int brightness = LED_BRIGHTNESS; brightness >= 0; brightness -= 10) {
      pixels.clear();
      pixels.setBrightness(brightness);
      pixels.setPixelColor(led, INDEX_PALETTE[led % 8]);
      pixels.show();
      delay(20);
    }
  }
  pixels.setBrightness(LED_BRIGHTNESS);  // Helligkeit zurücksetzen
  pixels.clear();
  pixels.show();
  
  // Serial Monitor starten
  Serial.begin(9600);
  // Midi Serial über TX1/RX1
  Serial1.begin(31250);
  // Timeout-Wartezeit statt Blockieren: erlaubt Standalone-Betrieb
  unsigned long serialTimeout = millis() + 2000;  // 2 Sekunden warten
  while (!Serial && millis() < serialTimeout) {
    ; // Warte kurz auf Serial Connection
  }
  
  
  // Debug: Zeige initiale Pin-Zustände
}

// MIDI Note On/Off Funktion
void noteOn(int cmd, int pitch, int velocity) {
  // Verfolge MIDI-Noten Status
  if (pitch >= 0 && pitch < 128) {
    if (velocity > 0) {
      // Note On
      activeMidiNotes[pitch] = true;
      disableControllerLEDsForNotes();
    } else {
      // Note Off
      activeMidiNotes[pitch] = false;
    }
  }
  
  Serial1.write(cmd);
  Serial1.write(pitch);
  Serial1.write(velocity);
}

// LED Kontrolle für Switch-Index mit optionaler Farbe
void setLEDWithColor(int switchIndex, bool on, uint32_t customColor = 0xFFFFFF, bool skipLEDs = false) {
  // Wenn skipLEDs true ist, setze keine LEDs (z.B. für Submenu Previews)
  if (skipLEDs) {
    return;
  }
  
  int ledIndex = ledMapping[switchIndex];
  
  // Nur wenn dieser Switch eine LED hat
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    if (on) {
      pixels.setPixelColor(ledIndex, customColor);
      pixels.show();  // Zeige die Änderung sofort an
    } else {
      // Vor dem Ausschalten prüfen, ob noch andere aktive Töne diese LED steuern
      bool ledStillActive = false;
      int otherSwitchIndex = -1;
      
      // Prüfe normale Switches
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (i != switchIndex && switches[i].isDown() && ledMapping[i] == ledIndex) {
          ledStillActive = true;
          otherSwitchIndex = i;
          break;
        }
      }
      
      // Prüfe aktive Akkord-Noten
      if (!ledStillActive && chordModeType != CHORD_MODE_OFF) {
        for (int i = 0; i < NUM_SWITCHES; i++) {
          if (i != switchIndex && chordNotesActive[i] && ledMapping[i] == ledIndex) {
            ledStillActive = true;
            otherSwitchIndex = i;
            break;
          }
        }
      }
      
      if (ledStillActive && otherSwitchIndex >= 0) {
        // Eine andere Note steuert diese LED noch - erneuere ihre Farbe
        setLED(otherSwitchIndex, true, skipLEDs);
      } else {
        // Keine andere Note auf dieser LED, LED ausschalten
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
        pixels.show();  // Zeige die Änderung sofort an
      }
    }
  }
}

// LED Kontrolle für Switch-Index (weiße und schwarze Tasten mit unterschiedlichen Farben)
void setLED(int switchIndex, bool on, bool skipLEDs = false) {
  // Wenn skipLEDs true ist, setze keine LEDs (z.B. für Submenu Previews)
  if (skipLEDs) {
    return;
  }
  
  int ledIndex = ledMapping[switchIndex];
  
  // Nur wenn dieser Switch eine LED hat
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    if (on) {
      // Wähle Farbe basierend auf Tastentyp
      uint32_t color;
      
      if (isBlackKey[switchIndex]) {
        // Schwarze Taste: Nutze BLACK_KEY_COLOR direkt (wird automatisch zu GRB konvertiert)
        color = BLACK_KEY_COLOR;
      } else {
        // Weiße Taste: Nutze WHITE_KEY_COLOR direkt (wird automatisch zu GRB konvertiert)
        color = WHITE_KEY_COLOR;
      }
      pixels.setPixelColor(ledIndex, color);
      
      // Update Multi-Note Tracking
      ledMultiNoteCount[ledIndex] = 0;
      ledMultiNoteSwitches[ledIndex][0] = -1;
      ledMultiNoteSwitches[ledIndex][1] = -1;
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (ledMapping[i] == ledIndex && (switches[i].isDown() || heldNotes[i])) {
          if (ledMultiNoteCount[ledIndex] < 2) {
            ledMultiNoteSwitches[ledIndex][ledMultiNoteCount[ledIndex]] = i;
          }
          ledMultiNoteCount[ledIndex]++;
        }
      }
    } else {
      // Vor dem Ausschalten prüfen, ob noch andere Switches diese LED steuern
      // Prüfe sowohl gedrückte Tasten als auch gehaltene Noten (für additiven Hold Mode)
      int otherActiveSwitch = -1;
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (i != switchIndex && ledMapping[i] == ledIndex) {
          // Prüfe ob Taste gedrückt oder Note gehälten wird
          if (switches[i].isDown() || heldNotes[i]) {
            otherActiveSwitch = i;
            break;
          }
        }
      }
      
      if (otherActiveSwitch != -1) {
        // Andere Taste steuert die gleiche LED, ihre Farbe anwenden
        uint32_t color;
        
        if (isBlackKey[otherActiveSwitch]) {
          color = BLACK_KEY_COLOR;
        } else {
          color = WHITE_KEY_COLOR;
        }
        pixels.setPixelColor(ledIndex, color);
        
        // Update Multi-Note Tracking
        ledMultiNoteCount[ledIndex] = 0;
        ledMultiNoteSwitches[ledIndex][0] = -1;
        ledMultiNoteSwitches[ledIndex][1] = -1;
        for (int i = 0; i < NUM_SWITCHES; i++) {
          if (ledMapping[i] == ledIndex && (switches[i].isDown() || heldNotes[i])) {
            if (ledMultiNoteCount[ledIndex] < 2) {
              ledMultiNoteSwitches[ledIndex][ledMultiNoteCount[ledIndex]] = i;
            }
            ledMultiNoteCount[ledIndex]++;
          }
        }
      } else {
        // Keine andere Taste auf dieser LED, LED ausschalten
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
        
        // Reset Multi-Note Tracking für diese LED
        ledMultiNoteCount[ledIndex] = 0;
        ledMultiNoteSwitches[ledIndex][0] = -1;
        ledMultiNoteSwitches[ledIndex][1] = -1;
        
        // Prüfe sofort ob zurück in Controller-Modus gewechselt werden soll
        bool hasActiveNotes = false;
        for (int i = 0; i < NUM_SWITCHES; i++) {
          if (heldNotes[i] || chordNotesActive[i] || switches[i].isDown()) {
            hasActiveNotes = true;
            break;
          }
        }
        
        // Wenn keine Noten mehr aktiv und nicht bereits im Controller-Modus
        if (!hasActiveNotes && !ledControllerMode && confirmationSwitchIndex < 0 && !octaveLEDActive) {
          ledControllerMode = true;
          // Verzögere Controller-LED Update minimal um flackern zu vermeiden
          updateControllerLEDs();
        }
      }
    }
    pixels.show();
  }
}

// LED-Anzeige für Oktave-Wechsel (LED 1-8 entspricht Oktave 0-7)
void showOctaveLED(int octave) {
  if (octave < 0 || octave > 7) return;  // Gültige Oktaven: 0-7
  
  // Aktiviere Oktave-LED Anzeige
  octaveLEDActive = true;
  octaveLEDStartTime = millis();
  isIdle = false;  // Nicht idle während Oktave-Anzeige
  
  // Alle LEDs ausschalten
  pixels.clear();
  
  // LED für aktuelle Oktave leuchten lassen (bereits in GRB)
  pixels.setPixelColor(octave, INDEX_PALETTE[octave % 8]);
  pixels.show();
}

// Erhöhe Oktave
void incrementOctave() {
  if (currentOctave < 7) {  // MIDI max Oktave 7
    currentOctave++;
    Serial.print("Oktave erhöht auf: ");
    Serial.println(currentOctave);
    showOctaveLED(currentOctave);  // LED-Anzeige für neue Oktave
  }
}

// Senke Oktave
void decrementOctave() {
  if (currentOctave > 0) {  // MIDI min Oktave 0
    currentOctave--;
    showOctaveLED(currentOctave);  // LED-Anzeige für neue Oktave
  }
}

// Toggle Akkord Modus (zyklisiert zwischen off -> extended -> folded -> off)
void toggleChordMode() {
  // Alle aktiven Akkord-Noten ausschalten
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (chordNotesActive[i]) {
      playChord(i, false);
      chordNotesActive[i] = false;
    }
  }
  
  // Zyklisiere durch die Modi
  chordModeType = (chordModeType + 1) % 3;  // 0 -> 1 -> 2 -> 0
  
}

// Helper-Funktion um Akkordnoten abzuschalten mit LED-Management
void turnOffChordNotes(int switchIndex, bool isFolded, bool skipLEDs = false) {
  int baseNote = midiNotes[switchIndex] + (currentOctave * 12);
  
  for (int j = 0; j < maxChordNotes; j++) {
    int noteOffset = getChordNote(switchIndex, scaleType, j);
    if (noteOffset >= 0) {
      int chordNote = baseNote + noteOffset;
      
      // Falte/passe Note an je nach Modus
      if (isFolded) {
        while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
        while (chordNote < currentOctave * 12) chordNote += 12;
      }
      
      noteOn(0x90, chordNote, 0x00);  // Note Off
      
      // Berechne Display-Index
      int displaySwitchIndex = (chordNote == (currentOctave + 1) * 12) ? 12 : (chordNote % 12);
      
      if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
        // Prüfe ob andere Akkorde diese LED steuern
        bool ledStillActive = false;
        for (int i = 0; i < NUM_SWITCHES; i++) {
          if (i != switchIndex && chordNotesActive[i]) {
            for (int k = 0; k < maxChordNotes; k++) {
              int offset = getChordNote(i, scaleType, k);
              if (offset >= 0) {
                int otherNote = midiNotes[i] + (currentOctave * 12) + offset;
                if (isFolded) {
                  while (otherNote > (currentOctave + 1) * 12) otherNote -= 12;
                  while (otherNote < currentOctave * 12) otherNote += 12;
                }
                int otherDisplayIndex = (otherNote == (currentOctave + 1) * 12) ? 12 : (otherNote % 12);
                if (otherDisplayIndex == displaySwitchIndex) {
                  ledStillActive = true;
                  break;
                }
              }
            }
            if (ledStillActive) break;
          }
        }
        if (!ledStillActive) setLEDWithColor(displaySwitchIndex, false, 0xFFFFFF, skipLEDs);
      }
    }
  }
}

// Helper-Funktion um Akkordnoten anzuschalten mit LED-Ansteuerung
void turnOnChordNotes(int switchIndex, bool isFolded, bool skipLEDs = false) {
  int baseNote = midiNotes[switchIndex] + (currentOctave * 12);
  
  // Bestimme Akkordtyp
  int chordDefIndex;
  if (scaleType >= 0 && scaleType <= 6) {
    chordDefIndex = getDiatonicChordType(switchIndex);
  } else if (scaleType == 7) {
    chordDefIndex = 2;  // Power 5
  } else {
    chordDefIndex = 3;  // Power 8
  }
  
  // Debug-Output
  //Serial.print("Switch ");
  //Serial.print(switchIndex);
  //Serial.print(" - Akkord: ");
  //switch(chordDefIndex) {
  //  case 0: Serial.println("Major"); break;
  //  case 1: Serial.println("Minor"); break;
  //  case 2: Serial.println("Power 5"); break;
  //  case 3: Serial.println("Power 8"); break;
  //  case 4: Serial.println("Sus4"); break;
  //  case 5: Serial.println("Augmented"); break;
  //  case 6: Serial.println("Diminished"); break;
  //  default: Serial.println("Unknown"); break;
  //}
  
  const uint32_t OUT_OF_RANGE_COLOR = 0x0000FF;  // Blau
  
  for (int j = 0; j < maxChordNotes; j++) {
    int noteOffset = getChordNote(switchIndex, scaleType, j);
    if (noteOffset >= 0) {
      int chordNote = baseNote + noteOffset;
      
      // Falte/passe Note an je nach Modus
      if (isFolded) {
        while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
        while (chordNote < currentOctave * 12) chordNote += 12;
      }
      
      // Prüfe ob Note außerhalb des Bereichs ist
      bool isOutOfRange = (chordNote < currentOctave * 12) || (chordNote > (currentOctave + 1) * 12);
      
      noteOn(0x90, chordNote, 0x45);  // Note On mit velocity
      
      // Berechne Display-Index
      int displaySwitchIndex = (chordNote == (currentOctave + 1) * 12) ? 12 : (chordNote % 12);
      
      if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
        if (!isFolded && isOutOfRange) {
          setLEDWithColor(displaySwitchIndex, true, OUT_OF_RANGE_COLOR, skipLEDs);
        } else {
          setLED(displaySwitchIndex, true, skipLEDs);
        }
      }
    }
  }
}

// Spiele Akkord mit Octave Folding (Töne werden in der Range gehalten)
void playChordFolded(int switchIndex, bool on, bool skipLEDs = false) {
  // Bei diatonischen Modi (0-6): Prüfe ob Taste diatonisch ist
  if (scaleType >= 0 && scaleType <= 6 && !isDiatonicNote(switchIndex)) {
    // Nicht-diatonische Taste - ignorieren
    if (on) {
      int ledIndex = ledMapping[switchIndex];
      pixels.setPixelColor(ledIndex, pixels.Color(0, 20, 0)); // Rot
      pixels.show();
      delay(100);
      pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0)); // Aus
      pixels.show();
    }
    return;
  }
  
  if (!on) {
    turnOffChordNotes(switchIndex, true, skipLEDs);
  } else {
    turnOnChordNotes(switchIndex, true, skipLEDs);
  }
}

// Spiele Akkord für einen Switch (on=true) oder schalte ihn aus (on=false)
void playChord(int switchIndex, bool on, bool skipLEDs = false) {
  // Bei diatonischen Modi (0-6): Prüfe ob Taste diatonisch ist
  if (scaleType >= 0 && scaleType <= 6 && !isDiatonicNote(switchIndex)) {
    // Nicht-diatonische Taste - ignorieren
    if (on) {
      int ledIndex = ledMapping[switchIndex];
      pixels.setPixelColor(ledIndex, pixels.Color(0, 20, 0)); // Rot
      pixels.show();
      delay(100);
      pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0)); // Aus
      pixels.show();
    }
    return;
  }
  
  if (!on) {
    turnOffChordNotes(switchIndex, false, skipLEDs);
  } else {
    turnOnChordNotes(switchIndex, false, skipLEDs);
  }
}

// Verarbeite Funktions-Schalter
void handleFunctionSwitches() {
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    
    // Trigger gibt nur einmal true beim Drücken zurück (wenn Pin LOW wird)
    if (functionSwitches[i].trigger()) {
      // Schalter wurde gerade gedrückt
      functionSwitchPressTime[i] = millis();  // Drückzeit speichern
      functionSwitchLongPressed[i] = false;   // Long-Press Flag zurücksetzen
      
      Serial.print("Function Switch ");
      Serial.print(i + 1);
      Serial.println(" PRESSED");
    }
    
    // Prüfe Long-Press (1000ms) während Schalter gehalten wird
    if (functionSwitches[i].isDown() && !inSubmenu && !functionSwitchLongPressed[i]) {
      unsigned long currentPressTime = millis() - functionSwitchPressTime[i];
      if (currentPressTime >= LONG_PRESS_DURATION) {
        // Long-Press erkannt - öffne entsprechendes Submenu
        functionSwitchLongPressed[i] = true;  // Flag setzen
        // FS3 (i=2) ist reserviert, nur FS1, FS2, FS4 öffnen Submenus
        if (i != 2) {
          enterSubmenu(i + 1);
        }
      }
    }
    
    // Prüfe ob Schalter losgelassen wurde mit Button Library Release-Erkennung
    if (functionSwitches[i].released()) {
      unsigned long pressDuration = millis() - functionSwitchPressTime[i];
      
    //  Serial.print("Function Switch ");
    //  Serial.print(i + 1);
    //  Serial.print(" RELEASED (Duration: ");
    //  Serial.print(pressDuration);
    //  Serial.println("ms)");
      
      // Nur Short-Press verarbeiten wenn kein Long-Press ausgeführt wurde
      if (!functionSwitchLongPressed[i]) {
        handleShortPress(i + 1);
      }
      
      // Flag zurücksetzen
      functionSwitchLongPressed[i] = false;
    }
  }
}

// Short-Press Handler für Funktionstasten
void handleShortPress(int fsNumber) {
  if (inSubmenu) {
    // Im Submenu: Navigation und Aktionen
    switch(fsNumber) {
      case 1: // FS1: Cancel
        exitSubmenu(false);
        break;
      case 2: // FS2: Save & Apply
        exitSubmenu(true);
        break;
      case 3: // FS3: Index runter
        if (submenuIndex > 0) {
          submenuIndex--;
          submenuChanged = true;
          // Für Submenu 4: Aktualisiere Oktave live für Vorschau
          if (currentSubmenu == 4) {
            currentOctave = submenuIndex;
          }
          updateControllerLEDs();
        }
        break;
      case 4: // FS4: Index hoch
        if (submenuIndex < maxSubmenuIndex - 1) {
          submenuIndex++;
          submenuChanged = true;
          // Für Submenu 4: Aktualisiere Oktave live für Vorschau
          if (currentSubmenu == 4) {
            currentOctave = submenuIndex;
          }
          updateControllerLEDs();
        }
        break;
    }
  } else {
    // Hauptmenü: Ein/Aus-Funktionen (nicht Toggle)
    switch(fsNumber) {
      case 1: // FS1: Play Mode Ein/Aus
        togglePlayModeOnOff();
        break;
      case 2: // FS2: Chord Mode Ein/Aus
        toggleChordModeOnOff();
        break;
      case 3: // FS3: Nicht verwendet im Hauptmenü (Long Press für Submenu 3)
        break;
      case 4: // FS4: Nicht verwendet im Hauptmenü (Long Press für Submenu 4)
        break;
    }
  }
}

// Submenu betreten
void enterSubmenu(int submenuNumber) {
  inSubmenu = true;
  currentSubmenu = submenuNumber;
  submenuIndex = 0;
  submenuChanged = true;
  isIdle = true;  // Sorge dafür, dass Controller-LEDs angezeigt werden
  
  switch(submenuNumber) {
    case 1: // Play Mode Submenu
      maxSubmenuIndex = 2; // Hold, Hold+Additive
      submenuIndex = playModeType;
      break;
    case 2: // Chord Mode Submenu
      maxSubmenuIndex = NUM_SCALE_TYPES;
      submenuIndex = scaleType;
      break;
    case 3: // Submenu 3 (noch nicht implementiert)
      maxSubmenuIndex = 1;
      break;
    case 4: // Octave Submenu (Oktaven 0-7)
      maxSubmenuIndex = 8; // Oktaven 0-7
      submenuIndex = currentOctave;
      savedOctaveBeforeSubmenu = currentOctave;  // Speichere ursprüngliche Oktave
      break;
  }
  
  Serial.print("Submenu ");
  Serial.print(submenuNumber);
  Serial.println(" geöffnet");
  updateControllerLEDs();
}

// Submenu verlassen
void exitSubmenu(bool saveChanges) {
  if (saveChanges) {
    switch(currentSubmenu) {
      case 1: // Play Mode Submenu
        if (submenuIndex != playModeType) {
          playModeType = submenuIndex;
          // Aktualisiere entsprechende Modi (nur wenn Play Mode aktiv ist)
          if (playModeActive) {
            holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
            additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
          }
          Serial.print("Play Mode Type gesetzt auf: ");
          switch(playModeType) {
            case PLAY_MODE_HOLD: Serial.println("HOLD"); break;
            case PLAY_MODE_ADDITIVE: Serial.println("HOLD + ADDITIVE"); break;
          }
        }
        break;
      case 2: // Chord Mode Submenu
        if (submenuIndex != scaleType) {
          scaleType = submenuIndex;
          Serial.print("Scale Type gesetzt auf: ");
          Serial.println(scaleType);
        }
        break;
      case 4: // Octave Submenu
        // currentOctave wurde bereits live aktualisiert während Navigation
        showOctaveLED(currentOctave);
        Serial.print("Oktave gespeichert: ");
        Serial.println(currentOctave);
        break;
    }
  } else {
    // Bei Cancel (saveChanges=false): Stelle ursprüngliche Werte wieder her
    switch(currentSubmenu) {
      case 4: // Octave Submenu - Stelle ursprüngliche Oktave wieder her
        currentOctave = savedOctaveBeforeSubmenu;
        Serial.print("Oktave zurückgesetzt auf: ");
        Serial.println(currentOctave);
        break;
    }
  }
  
  inSubmenu = false;
  currentSubmenu = 0;
  submenuIndex = 0;
  submenuChanged = true;
  Serial.println("Submenu verlassen");
  updateControllerLEDs();
}

// Toggle Play Mode Ein/Aus (FS1 Short Press)
void togglePlayModeOnOff() {
  playModeActive = !playModeActive;
  
  // Bei Deaktivierung: Alle gehaltenen Noten ausschalten
  if (!playModeActive) {
    // Standard Hold Mode ausschalten
    if (heldNote != -1) {
      noteOn(0x90, heldNote, 0x00);
      heldNote = -1;
    }
    // Additive Mode ausschalten  
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (heldNotes[i]) {
        int note = midiNotes[i] + (currentOctave * 12);
        noteOn(0x90, note, 0x00);
        heldNotes[i] = false;
        setLED(i, false);
      }
    }
    // Entsprechende Modi zurücksetzen
    holdMode = false;
    additiveMode = false;
  } else {
    // Bei Aktivierung: Modi basierend auf playModeType setzen (Default: Hold)
    holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
    additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
  }
  
  Serial.print("Play Mode: ");
  Serial.println(playModeActive ? "EIN" : "AUS");
  updateControllerLEDs();
}

// Toggle Chord Mode Ein/Aus (FS2 Short Press)
void toggleChordModeOnOff() {
  chordModeActive = !chordModeActive;
  
  // Bei Aktivierung: Setze einen gültigen Chord Mode Type wenn er noch auf OFF steht
  if (chordModeActive && chordModeType == CHORD_MODE_OFF) {
    chordModeType = CHORD_MODE_EXTENDED;  // Standard: Extended Mode
  }
  
  // Bei Deaktivierung: Alle aktiven Akkord-Noten ausschalten
  if (!chordModeActive) {
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (chordNotesActive[i]) {
        if (chordModeType == CHORD_MODE_EXTENDED) {
          playChord(i, false);
        } else if (chordModeType == CHORD_MODE_FOLDED) {
          playChordFolded(i, false);
        }
        chordNotesActive[i] = false;
      }
    }
  }
  
  Serial.print("Chord Mode: ");
  Serial.println(chordModeActive ? "EIN" : "AUS");
  updateControllerLEDs();
}

// Toggle Play Mode Type (für Submenu - deprecated, wird nicht mehr verwendet)
void togglePlayMode() {
  playModeType = (playModeType + 1) % 2;  // 0 -> 1 -> 0
  
  // Nur aktualisieren wenn Play Mode aktiv ist
  if (playModeActive) {
    holdMode = (playModeType == PLAY_MODE_HOLD || playModeType == PLAY_MODE_ADDITIVE);
    additiveMode = (playModeType == PLAY_MODE_ADDITIVE);
  }
  
  Serial.print("Play Mode Type: ");
  switch(playModeType) {
    case PLAY_MODE_HOLD: Serial.println("HOLD"); break;
    case PLAY_MODE_ADDITIVE: Serial.println("HOLD + ADDITIVE"); break;
  }
  updateControllerLEDs();
}

// Toggle Hold Modus
void toggleHoldMode() {
  // Alle gehaltenen Noten ausschalten
  if (!additiveMode && heldNote != -1) {
    noteOn(0x90, heldNote, 0x00);
    Serial.print("Hold Note Off: ");
    Serial.println(heldNote);
    // LEDs für alle derzeit gedrückten Switches ausschalten
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (switches[i].isDown()) {
        setLED(i, false);
      }
    }
    heldNote = -1;
  } else if (additiveMode) {
    // Im additiven Modus alle gehaltenen Noten ausschalten
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (heldNotes[i]) {
        int note = midiNotes[i] + (currentOctave * 12);
        noteOn(0x90, note, 0x00);
        Serial.print("Hold Note Off (Additive): ");
        Serial.println(note);
        heldNotes[i] = false;
        setLED(i, false);
      }
    }
  }
  
  holdMode = !holdMode;
  Serial.print("Hold Modus: ");
  Serial.println(holdMode ? "ON" : "OFF");
}

// Toggle Additive Modus
void toggleAdditiveMode() {
  // Alle gehaltenen Noten ausschalten
  if (!additiveMode && heldNote != -1) {
    noteOn(0x90, heldNote, 0x00);
    // LEDs ausschalten
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (switches[i].isDown()) {
        setLED(i, false);
      }
    }
    heldNote = -1;
  } else if (additiveMode) {
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (heldNotes[i]) {
        int note = midiNotes[i] + (currentOctave * 12);
        noteOn(0x90, note, 0x00);
        heldNotes[i] = false;
        setLED(i, false);
      }
    }
  }
  
  additiveMode = !additiveMode;
  Serial.print("Additive Modus: ");
  Serial.println(additiveMode ? "ON" : "OFF");
}

// Setze gehaltenen Akkord (für Hold + Chord Modus kombiniert)
void setHeldChord(int switchIndex, bool skipLEDs = false) {
  // Prüfe ob dieser Akkord bereits aktiv ist (Toggle)
  if (chordNotesActive[switchIndex]) {
    // Akkord ausschalten
    if (chordModeType == CHORD_MODE_EXTENDED) {
      playChord(switchIndex, false, skipLEDs);
    } else if (chordModeType == CHORD_MODE_FOLDED) {
      playChordFolded(switchIndex, false, skipLEDs);
    }
    chordNotesActive[switchIndex] = false;
  } else {
    // Alle anderen aktiven Akkorde ausschalten
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (i != switchIndex && chordNotesActive[i]) {
        if (chordModeType == CHORD_MODE_EXTENDED) {
          playChord(i, false, skipLEDs);
        } else if (chordModeType == CHORD_MODE_FOLDED) {
          playChordFolded(i, false, skipLEDs);
        }
        chordNotesActive[i] = false;
        Serial.print("Held Chord Off (Switch ");
        Serial.print(i);
        Serial.println(")");
      }
    }
    
    // Neuen Akkord anschalten
    if (chordModeType == CHORD_MODE_EXTENDED) {
      playChord(switchIndex, true, skipLEDs);
    } else if (chordModeType == CHORD_MODE_FOLDED) {
      playChordFolded(switchIndex, true, skipLEDs);
    }
    chordNotesActive[switchIndex] = true;
    Serial.print("Held Chord On (Switch ");
    Serial.print(switchIndex);
    Serial.println(")");
  }
}

// Setze gehaltene Note
void setHeldNote(int note, int switchIndex, bool skipLEDs = false) {
  if (additiveMode) {
    // Additive Modus: mehrere Noten gleichzeitig
    if (heldNotes[switchIndex]) {
      // Note ausschalten
      noteOn(0x90, note, 0x00);
      Serial.print("Hold Note Off (Additive): ");
      Serial.println(note);
      setLED(switchIndex, false, skipLEDs);
      heldNotes[switchIndex] = false;
    } else {
      // Note anschalten
      noteOn(0x90, note, 0x45);
      Serial.print("Hold Note On (Additive): ");
      Serial.println(note);
      setLED(switchIndex, true, skipLEDs);
      heldNotes[switchIndex] = true;
    }
  } else {
    // Standard Hold Modus: eine Note nach der anderen
    if (heldNote != -1 && heldNote != note) {
      // Alte Note ausschalten
      noteOn(0x90, heldNote, 0x00);
      Serial.print("Hold Note Off: ");
      Serial.println(heldNote);
      // LED für alte Note ausschalten (finde den Switch-Index)
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if ((midiNotes[i] + (currentOctave * 12)) == heldNote) {
          setLED(i, false, skipLEDs);
          break;
        }
      }
    }
    
    // Neue Note anschalten oder deaktivieren
    if (note == heldNote) {
      // Toggle - gleiche Note wurde erneut gedrückt
      noteOn(0x90, note, 0x00);
      Serial.print("Hold Note Off: ");
      Serial.println(note);
      setLED(switchIndex, false, skipLEDs);
      heldNote = -1;
    } else {
      // Neue Note anschalten
      setLED(switchIndex, true, skipLEDs);
      heldNote = note;
      noteOn(0x90, note, 0x45);
      Serial.print("Hold Note On: ");
      Serial.println(note);
    }
  }
}

void loop() {
  // Alle Switches durchgehen
  for (int i = 0; i < NUM_SWITCHES; i++) {
    
    // Trigger gibt nur einmal true beim Drücken zurück (wenn Pin LOW wird)
    if (switches[i].trigger()) {
      // Switch wurde gerade gedrückt
      int currentNote = midiNotes[i] + (currentOctave * 12);
      
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.print("): PRESSED - Note ");
      Serial.println(currentNote);
      
      // Im Submenu 1 (Play Mode): Noten-Vorschau spielen
      if (inSubmenu && currentSubmenu == 1) {
        // Spiele die Note zur Vorschau
        noteOn(0x90, currentNote, 0x45);
        return; // Verlasse loop iteration, normale Verarbeitung überspringen
      }
      
      // Im Submenu 2 (Chord Mode): Root Note Auswahl bei Short Press
      if (inSubmenu && currentSubmenu == 2) {
        diatonicRootKey = midiNotes[i];
        confirmLED(i); // Bestätigungs-Blinken
        Serial.print("Root Key gesetzt auf: ");
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        Serial.println(noteNames[diatonicRootKey]);
        return; // Verlasse loop iteration, normale Note nicht spielen
      }
      
      // Im Submenu 3 (reserviert): Noten-Vorschau spielen
      if (inSubmenu && currentSubmenu == 3) {
        // Spiele die Note zur Vorschau
        noteOn(0x90, currentNote, 0x45);
        return; // Verlasse loop iteration, normale Verarbeitung überspringen
      }
      
      // Im Submenu 4 (Octave): Vorschau mit aktiven Modi (Chord Mode, Hold Mode)
      if (inSubmenu && currentSubmenu == 4) {
        // Wenn Hold Mode aktiv: halte Noten mit skipLEDs=true (keine LEDs setzen)
        if (holdMode) {
          if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
            // Hold + Chord: Akkord halten
            setHeldChord(i, true);  // skipLEDs=true
          } else {
            // Hold Normal: Note halten
            setHeldNote(currentNote, i, true);  // skipLEDs=true
          }
        } else {
          // Kein Hold Mode: Nur Vorschau
          if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
            int baseNote = midiNotes[i] + (currentOctave * 12);
            // Spiele Akkord-Noten direkt (ohne setLEDWithColor)
            for (int j = 0; j < maxChordNotes; j++) {
              int noteOffset = getChordNote(i, scaleType, j);
              if (noteOffset >= 0) {
                int chordNote = baseNote + noteOffset;
                if (chordModeType == CHORD_MODE_FOLDED) {
                  while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
                  while (chordNote < currentOctave * 12) chordNote += 12;
                }
                noteOn(0x90, chordNote, 0x45);  // Note On
              }
            }
          } else {
            // Normal: Spiele einfache Note in der ausgewählten Oktave zur Vorschau
            noteOn(0x90, currentNote, 0x45);
          }
        }
        return; // Verlasse loop iteration, normale Verarbeitung überspringen
      }
      
      // Akkord Modus: mehrere Noten gleichzeitig
      if (chordModeActive && chordModeType != CHORD_MODE_OFF) {
        disableControllerLEDsForNotes();  // Controller-LEDs ausschalten
        if (holdMode) {
          // Akkord-Hold Modus: Akkord wird geholt (alte Akkorde ausgelöst)
          setHeldChord(i);
        } else {
          // Akkord Normal Modus: Akkord solange gedrückt
          if (chordModeType == CHORD_MODE_EXTENDED) {
            playChord(i, true);
          } else if (chordModeType == CHORD_MODE_FOLDED) {
            playChordFolded(i, true);
          }
          chordNotesActive[i] = true;
        }
      }
      // Hold Modus: Note halten bis zum nächsten Drücken
      else if (holdMode) {
        disableControllerLEDsForNotes();  // Controller-LEDs ausschalten
        setLED(i, true);
        setHeldNote(currentNote, i);
      } else {
        // Normal Modus: Note solange gedrückt
        disableControllerLEDsForNotes();  // Controller-LEDs ausschalten
        setLED(i, true);
        noteOn(0x90, currentNote, 0x45);
      }
    }
    
    // Prüfe ob Switch losgelassen wurde mit Button Library Release-Erkennung
    if (switches[i].released()) {
      // Switch wurde gerade losgelassen
      int currentNote = midiNotes[i] + (currentOctave * 12);
      
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.print("): RELEASED - Note ");
      Serial.println(currentNote);
      
      // Im Submenu: Note off zur Vorschau
      if (inSubmenu && currentSubmenu != 2) {
        // Submenu 4: Mit Hold Mode wird Release bereits durch den nächsten trigger() gehandhabt
        if (currentSubmenu == 4) {
          if (!holdMode && chordModeActive && chordModeType != CHORD_MODE_OFF) {
            // Nur Preview ohne Hold: Akkord ausschalten
            int baseNote = midiNotes[i] + (currentOctave * 12);
            for (int j = 0; j < maxChordNotes; j++) {
              int noteOffset = getChordNote(i, scaleType, j);
              if (noteOffset >= 0) {
                int chordNote = baseNote + noteOffset;
                
                if (chordModeType == CHORD_MODE_FOLDED) {
                  while (chordNote > (currentOctave + 1) * 12) chordNote -= 12;
                  while (chordNote < currentOctave * 12) chordNote += 12;
                }
                
                noteOn(0x90, chordNote, 0x00);  // Note Off
              }
            }
          } else if (!holdMode) {
            // Nur Preview ohne Hold: Note ausschalten
            noteOn(0x90, currentNote, 0x00);
          }
          // Mit Hold Mode: Release ist bereits durch den nächsten trigger() (zweiter Druck) gehandhabt
        } else {
          // Submenu 1, 3: Note off spielen
          noteOn(0x90, currentNote, 0x00);
        }
        return; // Verlasse loop iteration, normale Verarbeitung überspringen
      }
      
      // LED ausschalten (nur im Normal Modus und nicht im Akkord-Modus)
      if (!holdMode && (!chordModeActive || chordModeType == CHORD_MODE_OFF)) {
        setLED(i, false);
      }
      
      // Akkord Modus: Akkord ausschalten (nur wenn NOT in Hold-Modus)
      if (chordModeActive && chordModeType != CHORD_MODE_OFF && !holdMode) {
        if (chordModeType == CHORD_MODE_EXTENDED) {
          playChord(i, false);
        } else if (chordModeType == CHORD_MODE_FOLDED) {
          playChordFolded(i, false);
        }
        chordNotesActive[i] = false;
      }
      // Nur im Normal Modus die Note ausschalten
      else if (!holdMode && (!chordModeActive || chordModeType == CHORD_MODE_OFF)) {
        noteOn(0x90, currentNote, 0x00);
      }
    }
  }
  
  // Verarbeite Funktions-Schalter (A1-A4)
  // Verarbeite Function Switch Events (alle 4 Switches)
  handleFunctionSwitches();
  
  // Update Idle-Status
  updateIdleStatus();
  updateConfirmationBlink();
  updateLEDMultiNoteBlink();
}
