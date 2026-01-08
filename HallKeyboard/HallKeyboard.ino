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
const int LED_BRIGHTNESS = 50;       // LED Helligkeit (0-255)
const int WHITE_KEY_COLOR = 0xFFFFFF;   // Farbe für weiße Tasten
const int BLACK_KEY_COLOR = 0x8B8BFF;   // Farbe für schwarze Tasten
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
int scaleType = 0;                  // 0=Diatonic Major, 1=Diatonic Minor, 2=Power 5, 3=Power 8
#define NUM_SCALE_TYPES 4
bool diatonicIsMajor = true;        // true = Dur, false = Moll

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

// Diatonische Muster für Dur und Moll
// Index 0-6 entspricht den Stufen der Tonleiter (I-VII)
const int diatonicPatternMajor[7] = {0, 1, 1, 0, 0, 1, 6};  // Major, minor, minor, Major, Major, minor, Dim
const int diatonicPatternMinor[7] = {1, 6, 0, 1, 1, 0, 0};  // minor, Dim, Major, minor, minor, Major, Major

// Hilfsfunktion um diatonischen Akkordtyp zu berechnen
// switchIndex: 0-12 (C bis C)
// Gibt zurück welcher Akkordtyp (0-6) für diese Taste verwendet werden sollte
int getDiatonicChordType(int switchIndex) {
  // Berechne den Abstand vom Root Key
  int noteOffset = (midiNotes[switchIndex] - diatonicRootKey + 12) % 12;
  // Bestimme die Stufe (0-6) in der diatonischen Tonleiter
  // C-Dur-Tonleiter: C D E F G A B (0, 2, 4, 5, 7, 9, 11)
  int diatonicDegree = -1;
  
  if (noteOffset == 0) diatonicDegree = 0;  // I
  else if (noteOffset == 2) diatonicDegree = 1;  // ii
  else if (noteOffset == 4) diatonicDegree = 2;  // iii
  else if (noteOffset == 5) diatonicDegree = 3;  // IV
  else if (noteOffset == 7) diatonicDegree = 4;  // V
  else if (noteOffset == 9) diatonicDegree = 5;  // vi
  else if (noteOffset == 11) diatonicDegree = 6; // vii°
  else {
    // Schwarze Taste - verwende Major als Default
    return 0;
  }
  
  // Gebe den Akkordtyp basierend auf dem Muster zurück
  if (diatonicIsMajor) {
    return diatonicPatternMajor[diatonicDegree];
  } else {
    return diatonicPatternMinor[diatonicDegree];
  }
}

// Hilfsfunktion um die Akkordnoten zu erhalten
int getChordNote(int switchIndex, int variationType, int noteIndex) {
  int chordDefIndex;
  
  switch(variationType % NUM_SCALE_TYPES) {
    case 0:  // Diatonic Major
      diatonicIsMajor = true;
      chordDefIndex = getDiatonicChordType(switchIndex);
      break;
    case 1:  // Diatonic Minor
      diatonicIsMajor = false;
      chordDefIndex = getDiatonicChordType(switchIndex);
      break;
    case 2:  // Power 5
      chordDefIndex = 2;  // Power 5 (0, 7, -1)
      break;
    case 3:  // Power 8
      chordDefIndex = 3;  // Power 8 (0, 7, 12)
      break;
    default:
      chordDefIndex = 0;  // Major
  }
  
  return chordDefinitions[chordDefIndex][noteIndex];
}

bool chordNotesActive[NUM_SWITCHES];  // Speichert ob Akkord-Noten für einen Switch aktiv sind

// Funktions-Schalter (4 Schalter an A1-A4)
const int NUM_FUNCTION_SWITCHES = 4;
const int functionSwitchPins[NUM_FUNCTION_SWITCHES] = {
  A1, A2, A3, A4
};
Button functionSwitches[NUM_FUNCTION_SWITCHES];
unsigned long functionSwitchPressTime[NUM_FUNCTION_SWITCHES];  // Zeit beim Drücken speichern
const unsigned long LONG_PRESS_DURATION = 1000;  // 1 Sekunde für Long-Press

void setup() {
  // Alle Switch-Pins initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switches[i].begin(switchPins[i]);
    heldNotes[i] = false;
  }
  
  // Analog Pins als Digital mit Pullup ZUERST konfigurieren
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  
  // Funktions-Schalter initialisieren (A1-A4 als Digitalpins mit Pullup)
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    functionSwitches[i].begin(functionSwitchPins[i]);
  }
  
  // Akkord-Mode Array initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    chordNotesActive[i] = false;
  }
  
  // WS2812 LEDs initialisieren
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);  // Helligkeit zentral einstellen
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
  
  Serial.println("Hall-Effect Keyboard - 13 Switches");
  Serial.println("===================================");
  Serial.println("Pins: D2-D12, D17(A3), D18(A4)");
  Serial.println("Using Button library with bit-shift debouncing and state management");
  Serial.println("Zeigt nur State Changes an");
  Serial.println("4 Funktions-Schalter an A1-A4");
  Serial.println("8 WS2812 LEDs an A5");
  Serial.println("-----------------------------------");
  
  // Debug: Zeige initiale Pin-Zustände
  Serial.println("\nInitiale Pin-Zustände:");
  for (int i = 0; i < NUM_SWITCHES; i++) {
    int state = digitalRead(switchPins[i]);
    Serial.print("Switch ");
    Serial.print(i);
    Serial.print(" (Pin ");
    Serial.print(switchPins[i]);
    Serial.print("): ");
    Serial.println(state == HIGH ? "HIGH" : "LOW");
  }
  Serial.println("-----------------------------------\n");
  
  
  Serial.println("Funktions-Schalter mit Button Library initialisiert");
}

// MIDI Note On/Off Funktion
void noteOn(int cmd, int pitch, int velocity) {
  Serial1.write(cmd);
  Serial1.write(pitch);
  Serial1.write(velocity);
}

// LED Kontrolle für Switch-Index mit optionaler Farbe
void setLEDWithColor(int switchIndex, bool on, uint32_t customColor = 0xFFFFFF) {
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
        setLED(otherSwitchIndex, true);
      } else {
        // Keine andere Note auf dieser LED, LED ausschalten
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
        pixels.show();  // Zeige die Änderung sofort an
      }
    }
  }
}

// LED Kontrolle für Switch-Index (weiße und schwarze Tasten mit unterschiedlichen Farben)
void setLED(int switchIndex, bool on) {
  int ledIndex = ledMapping[switchIndex];
  
  // Nur wenn dieser Switch eine LED hat
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    if (on) {
      // Wähle Farbe basierend auf Tastentyp
      uint32_t color;
      
      if (isBlackKey[switchIndex]) {
        // Schwarze Taste: Farbe aus BLACK_KEY_COLOR (Helligkeit wird durch setBrightness() geregelt)
        uint8_t r = (BLACK_KEY_COLOR >> 16) & 0xFF;
        uint8_t g = (BLACK_KEY_COLOR >> 8) & 0xFF;
        uint8_t b = BLACK_KEY_COLOR & 0xFF;
        color = pixels.Color(r, g, b);
      } else {
        // Weiße Taste: Farbe aus WHITE_KEY_COLOR (Helligkeit wird durch setBrightness() geregelt)
        uint8_t r = (WHITE_KEY_COLOR >> 16) & 0xFF;
        uint8_t g = (WHITE_KEY_COLOR >> 8) & 0xFF;
        uint8_t b = WHITE_KEY_COLOR & 0xFF;
        color = pixels.Color(r, g, b);
      }
      pixels.setPixelColor(ledIndex, color);
    } else {
      // Vor dem Ausschalten prüfen, ob noch andere Switches diese LED steuern
      int otherActiveSwitch = -1;
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (i != switchIndex && switches[i].isDown() && ledMapping[i] == ledIndex) {
          otherActiveSwitch = i;
          break;
        }
      }
      
      if (otherActiveSwitch != -1) {
        // Andere Taste steuert die gleiche LED, ihre Farbe anwenden
        uint32_t color;
        
        if (isBlackKey[otherActiveSwitch]) {
          uint8_t r = (BLACK_KEY_COLOR >> 16) & 0xFF;
          uint8_t g = (BLACK_KEY_COLOR >> 8) & 0xFF;
          uint8_t b = BLACK_KEY_COLOR & 0xFF;
          color = pixels.Color(r, g, b);
        } else {
          uint8_t r = (WHITE_KEY_COLOR >> 16) & 0xFF;
          uint8_t g = (WHITE_KEY_COLOR >> 8) & 0xFF;
          uint8_t b = WHITE_KEY_COLOR & 0xFF;
          color = pixels.Color(r, g, b);
        }
        pixels.setPixelColor(ledIndex, color);
      } else {
        // Keine andere Taste auf dieser LED, LED ausschalten
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
      }
    }
    pixels.show();
  }
}

// Erhöhe Oktave
void incrementOctave() {
  if (currentOctave < 7) {  // MIDI max Oktave 7
    currentOctave++;
    Serial.print("Oktave erhöht auf: ");
    Serial.println(currentOctave);
  }
}

// Senke Oktave
void decrementOctave() {
  if (currentOctave > 0) {  // MIDI min Oktave 0
    currentOctave--;
    Serial.print("Oktave gesenkt auf: ");
    Serial.println(currentOctave);
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
  
  Serial.print("Akkord Modus: ");
  switch(chordModeType) {
    case CHORD_MODE_OFF:
      Serial.println("OFF");
      break;
    case CHORD_MODE_EXTENDED:
      Serial.println("EXTENDED (Töne über Range möglich)");
      break;
    case CHORD_MODE_FOLDED:
      Serial.println("FOLDED (Töne werden nach unten gefaltet)");
      break;
  }
}

// Spiele Akkord mit Octave Folding (Töne werden in der Range gehalten)
void playChordFolded(int switchIndex, bool on) {
  // Berechne die Basis-MIDI-Note für diesen Switch
  int baseNote = midiNotes[switchIndex] + (currentOctave * 12);
  
  if (!on) {
    // Akkord ausschalten
    for (int j = 0; j < maxChordNotes; j++) {
      int noteOffset = getChordNote(switchIndex, scaleType, j);
      if (noteOffset >= 0) {  // Nur wenn Note definiert (>= 0)
        int chordNote = baseNote + noteOffset;  // Addiere Semitone zur Basis-Note
        
        // Falte die Note in die Range: currentOctave*12 bis (currentOctave+1)*12
        while (chordNote > (currentOctave + 1) * 12) {
          chordNote -= 12;  // Eine Oktave nach unten
        }
        while (chordNote < currentOctave * 12) {
          chordNote += 12;  // Eine Oktave nach oben
        }
        
        noteOn(0x90, chordNote, 0x00);  // Note Off
        
        // Berechne LED-Index für die gefaltete Note und schalte sie aus
        int displaySwitchIndex;
        if (chordNote == (currentOctave + 1) * 12) {
          displaySwitchIndex = 12;
        } else {
          displaySwitchIndex = chordNote % 12;
        }
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          bool ledStillActive = false;
          for (int i = 0; i < NUM_SWITCHES; i++) {
            if (i != switchIndex && chordNotesActive[i]) {
              for (int k = 0; k < maxChordNotes; k++) {
                int offset = getChordNote(i, scaleType, k);
                if (offset >= 0) {
                  int otherNote = midiNotes[i] + (currentOctave * 12) + offset;
                  while (otherNote > (currentOctave + 1) * 12) {
                    otherNote -= 12;
                  }
                  while (otherNote < currentOctave * 12) {
                    otherNote += 12;
                  }
                  int otherDisplayIndex = otherNote % 12;
                  if (otherDisplayIndex == displaySwitchIndex) {
                    ledStillActive = true;
                    break;
                  }
                }
              }
              if (ledStillActive) break;
            }
          }
          
          if (!ledStillActive) {
            setLEDWithColor(displaySwitchIndex, false);
          }
        }
      }
    }
  } else {
    // Akkord anschalten
    int chordDefIndex;
    if (scaleType == 0) {
      diatonicIsMajor = true;
      chordDefIndex = getDiatonicChordType(switchIndex);
    } else if (scaleType == 1) {
      diatonicIsMajor = false;
      chordDefIndex = getDiatonicChordType(switchIndex);
    } else if (scaleType == 2) {
      chordDefIndex = 2;  // Power 5
    } else {
      chordDefIndex = 3;  // Power 8
    }
    
    // Gebe Akkordtyp aus
    Serial.print("Switch ");
    Serial.print(switchIndex);
    Serial.print(" - Akkord: ");
    switch(chordDefIndex) {
      case 0: Serial.println("Major"); break;
      case 1: Serial.println("Minor"); break;
      case 2: Serial.println("Power 5"); break;
      case 3: Serial.println("Power 8"); break;
      case 4: Serial.println("Sus4"); break;
      case 5: Serial.println("Augmented"); break;
      case 6: Serial.println("Diminished"); break;
      default: Serial.println("Unknown"); break;
    }
    
    for (int j = 0; j < maxChordNotes; j++) {
      int noteOffset = getChordNote(switchIndex, scaleType, j);
      if (noteOffset >= 0) {  // Nur wenn Note definiert (>= 0)
        int chordNote = baseNote + noteOffset;  // Addiere Semitone zur Basis-Note
        
        // Falte die Note in die Range: currentOctave*12 bis (currentOctave+1)*12
        while (chordNote > (currentOctave + 1) * 12) {
          chordNote -= 12;  // Eine Oktave nach unten
        }
        while (chordNote < currentOctave * 12) {
          chordNote += 12;  // Eine Oktave nach oben
        }
        
        noteOn(0x90, chordNote, 0x45);  // Note On mit velocity
        
        // Berechne LED-Index für die gefaltete Note
        int displaySwitchIndex;
        if (chordNote == (currentOctave + 1) * 12) {
          displaySwitchIndex = 12;
        } else {
          displaySwitchIndex = chordNote % 12;
        }
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          setLED(displaySwitchIndex, true);
        }
      }
    }
  }
}

// Spiele Akkord für einen Switch (on=true) oder schalte ihn aus (on=false)
void playChord(int switchIndex, bool on) {
  // Berechne die Basis-MIDI-Note für diesen Switch
  int baseNote = midiNotes[switchIndex] + (currentOctave * 12);
  
  // Farbe für Out-of-Range Noten (blau)
  const uint32_t OUT_OF_RANGE_COLOR = 0x0000FF;  // Blau
  
  if (!on) {
    // Akkord ausschalten
    for (int j = 0; j < maxChordNotes; j++) {
      int noteOffset = getChordNote(switchIndex, scaleType, j);
      if (noteOffset >= 0) {  // Nur wenn Note definiert (>= 0)
        int chordNote = baseNote + noteOffset;  // Addiere Semitone zur Basis-Note
        noteOn(0x90, chordNote, 0x00);  // Note Off
        
        // Berechne LED-Index basierend auf der Octave-Anpassung
        int octaveAdjustedNote = chordNote;
        bool isOutOfRange = false;
        
        // Wenn die Note außerhalb der aktuellen Octave liegt, passe sie an
        // Range ist currentOctave*12 bis (currentOctave+1)*12 inklusive
        if (chordNote > (currentOctave + 1) * 12) {
          octaveAdjustedNote = chordNote - 12;  // Eine Oktave tiefer
          isOutOfRange = true;
        } else if (chordNote < currentOctave * 12) {
          octaveAdjustedNote = chordNote + 12;  // Eine Oktave höher
          isOutOfRange = true;
        }
        
        // Berechne den Switch-Index aus der angepassten Note
        int displaySwitchIndex;
        if (chordNote == (currentOctave + 1) * 12) {
          // Das ist die obere C (C1 in der Tastatur) - verwende Switch 12
          displaySwitchIndex = 12;
        } else {
          displaySwitchIndex = octaveAdjustedNote % 12;
        }
        
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          // Prüfe, ob noch andere Akkorde diese LED steuern (außer diesem switchIndex)
          bool ledStillActive = false;
          for (int i = 0; i < NUM_SWITCHES; i++) {
            if (i != switchIndex && chordNotesActive[i]) {
              // Prüfe, ob dieser Akkord die gleiche LED steuert
              for (int k = 0; k < maxChordNotes; k++) {
                int offset = getChordNote(i, scaleType, k);
                if (offset >= 0) {
                  int otherNote = midiNotes[i] + (currentOctave * 12) + offset;
                  int otherOctaveAdjustedNote = otherNote;
                  if (otherNote >= (currentOctave + 1) * 12) {
                    otherOctaveAdjustedNote = otherNote - 12;
                  } else if (otherNote < currentOctave * 12) {
                    otherOctaveAdjustedNote = otherNote + 12;
                  }
                  int otherDisplayIndex = (otherNote == (currentOctave + 1) * 12) ? 12 : (otherOctaveAdjustedNote % 12);
                  if (otherDisplayIndex == displaySwitchIndex) {
                    ledStillActive = true;
                    break;
                  }
                }
              }
              if (ledStillActive) break;
            }
          }
          
          if (!ledStillActive) {
            setLEDWithColor(displaySwitchIndex, false);
          }
        }
      }
    }
  } else {
    // Akkord anschalten
    int chordDefIndex;
    if (scaleType == 0) {
      diatonicIsMajor = true;
      chordDefIndex = getDiatonicChordType(switchIndex);
    } else if (scaleType == 1) {
      diatonicIsMajor = false;
      chordDefIndex = getDiatonicChordType(switchIndex);
    } else if (scaleType == 2) {
      chordDefIndex = 2;  // Power 5
    } else {
      chordDefIndex = 3;  // Power 8
    }
    
    // Gebe Akkordtyp aus
    Serial.print("Switch ");
    Serial.print(switchIndex);
    Serial.print(" - Akkord: ");
    switch(chordDefIndex) {
      case 0: Serial.println("Major"); break;
      case 1: Serial.println("Minor"); break;
      case 2: Serial.println("Power 5"); break;
      case 3: Serial.println("Power 8"); break;
      case 4: Serial.println("Sus4"); break;
      case 5: Serial.println("Augmented"); break;
      case 6: Serial.println("Diminished"); break;
      default: Serial.println("Unknown"); break;
    }
    
    for (int j = 0; j < maxChordNotes; j++) {
      int noteOffset = getChordNote(switchIndex, scaleType, j);
      if (noteOffset >= 0) {  // Nur wenn Note definiert (>= 0)
        int chordNote = baseNote + noteOffset;  // Addiere Semitone zur Basis-Note
        noteOn(0x90, chordNote, 0x45);  // Note On mit velocity
        
        // Berechne LED-Index basierend auf der Octave-Anpassung
        int octaveAdjustedNote = chordNote;
        bool isOutOfRange = false;
        
        // Wenn die Note außerhalb der aktuellen Octave liegt, passe sie an
        // Range ist currentOctave*12 bis (currentOctave+1)*12 inklusive
        if (chordNote > (currentOctave + 1) * 12) {
          octaveAdjustedNote = chordNote - 12;  // Eine Oktave tiefer
          isOutOfRange = true;
        } else if (chordNote < currentOctave * 12) {
          octaveAdjustedNote = chordNote + 12;  // Eine Oktave höher
          isOutOfRange = true;
        }
        
        // Berechne den Switch-Index aus der angepassten Note
        int displaySwitchIndex;
        if (chordNote == (currentOctave + 1) * 12) {
          // Das ist die obere C (C1 in der Tastatur) - verwende Switch 12
          displaySwitchIndex = 12;
        } else {
          displaySwitchIndex = octaveAdjustedNote % 12;
        }
        
        if (displaySwitchIndex >= 0 && displaySwitchIndex < NUM_SWITCHES) {
          if (isOutOfRange) {
            setLEDWithColor(displaySwitchIndex, true, OUT_OF_RANGE_COLOR);
          } else {
            setLED(displaySwitchIndex, true);
          }
        }
      }
    }
  }
}

// Verarbeite Funktions-Schalter
void handleFunctionSwitches() {
  for (int i = 0; i < NUM_FUNCTION_SWITCHES; i++) {
    
    // Trigger gibt nur einmal true beim Drücken zurück (wenn Pin LOW wird)
    if (functionSwitches[i].trigger()) {
      // Schalter wurde gerade gedrückt
      functionSwitchPressTime[i] = millis();  // Drückzeit speichern
      
      Serial.print("Function Switch ");
      Serial.print(i + 1);
      Serial.println(" PRESSED");
      
      // Spezielle Behandlung für FS3 Trigger: prüfe FS2+FS3 Kombination
      if (i == 2 && functionSwitches[1].isDown()) {
        // FS2 wird noch gehalten + FS3 gerade gedrückt = Kombination
        // Inkrementiere Scale Type (mit Cycling)
        scaleType = (scaleType + 1) % NUM_SCALE_TYPES;
        
        Serial.print(">>> Scale Type gewechselt zu: ");
        switch(scaleType) {
          case 0: Serial.println("Diatonic Major"); break;
          case 1: Serial.println("Diatonic Minor"); break;
          case 2: Serial.println("Power 5"); break;
          case 3: Serial.println("Power 8"); break;
        }
        fs3CombinationHandled = true;  // Flag setzen um doppelte Verarbeitung zu verhindern
        fs2CombinationHandled = true;  // Flag auch für FS2 setzen um toggleChordMode() zu verhindern
      }
    }
    
    // Prüfe ob Schalter losgelassen wurde mit Button Library Release-Erkennung
    if (functionSwitches[i].released()) {
      unsigned long pressDuration = millis() - functionSwitchPressTime[i];
      
      Serial.print("Function Switch ");
      Serial.print(i + 1);
      Serial.print(" RELEASED (Duration: ");
      Serial.print(pressDuration);
      Serial.println("ms)");
      
      // Spezielle Behandlung für FS2 (Scale Type Schalter, Index 1)
      if (i == 1) {
        // FS2 allein: Toggle Akkord-Modus (nur wenn keine Kombination ausgeführt wurde)
        if (!fs2CombinationHandled) {
          toggleChordMode();
        } else {
          Serial.println(">>> FS2 Kombination - toggleChordMode() übersprungen");
        }
        // Flag zurücksetzen beim FS2 Release
        fs2CombinationHandled = false;
      } else if (i == 2) {
        // FS3 (A3) Handler - aber nur wenn nicht mit FS2 kombiniert
        if (!fs3CombinationHandled) {
          // FS3 allein: Oktave runter
          decrementOctave();
        } else {
          Serial.println(">>> FS3 Kombination - decrementOctave() übersprungen");
        }
        // Flag zurücksetzen beim FS3 Release
        fs3CombinationHandled = false;
      } else if (i == 3) {
        // FS4 (A4): Oktave rauf
        incrementOctave();
      } else if (i == 0) {
        // FS1 (A1): Toggle Hold Mode
        toggleHoldMode();
      }
    }
  }
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
void setHeldChord(int switchIndex) {
  // Prüfe ob dieser Akkord bereits aktiv ist (Toggle)
  if (chordNotesActive[switchIndex]) {
    // Akkord ausschalten
    if (chordModeType == CHORD_MODE_EXTENDED) {
      playChord(switchIndex, false);
    } else if (chordModeType == CHORD_MODE_FOLDED) {
      playChordFolded(switchIndex, false);
    }
    chordNotesActive[switchIndex] = false;
  } else {
    // Alle anderen aktiven Akkorde ausschalten
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (i != switchIndex && chordNotesActive[i]) {
        if (chordModeType == CHORD_MODE_EXTENDED) {
          playChord(i, false);
        } else if (chordModeType == CHORD_MODE_FOLDED) {
          playChordFolded(i, false);
        }
        chordNotesActive[i] = false;
        Serial.print("Held Chord Off (Switch ");
        Serial.print(i);
        Serial.println(")");
      }
    }
    
    // Neuen Akkord anschalten
    if (chordModeType == CHORD_MODE_EXTENDED) {
      playChord(switchIndex, true);
    } else if (chordModeType == CHORD_MODE_FOLDED) {
      playChordFolded(switchIndex, true);
    }
    chordNotesActive[switchIndex] = true;
    Serial.print("Held Chord On (Switch ");
    Serial.print(switchIndex);
    Serial.println(")");
  }
}

// Setze gehaltene Note
void setHeldNote(int note, int switchIndex) {
  if (additiveMode) {
    // Additive Modus: mehrere Noten gleichzeitig
    if (heldNotes[switchIndex]) {
      // Note ausschalten
      noteOn(0x90, note, 0x00);
      Serial.print("Hold Note Off (Additive): ");
      Serial.println(note);
      setLED(switchIndex, false);
      heldNotes[switchIndex] = false;
    } else {
      // Note anschalten
      noteOn(0x90, note, 0x45);
      Serial.print("Hold Note On (Additive): ");
      Serial.println(note);
      setLED(switchIndex, true);
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
          setLED(i, false);
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
      setLED(switchIndex, false);
      heldNote = -1;
    } else {
      // Neue Note anschalten
      setLED(switchIndex, true);
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
      
      // Prüfe ob Function2 (A2) gleichzeitig gedrückt ist
      if (functionSwitches[1].isDown()) {
        // Function2 + Taste = setze neuen diatonischen Grundton
        diatonicRootKey = midiNotes[i];
        diatonicIsMajor = true;
        
        Serial.print(">>> Diatonischer Root gesetzt auf: ");
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        Serial.println(noteNames[diatonicRootKey]);
        
        fs2CombinationHandled = true;  // Flag setzen um toggleChordMode() beim FS2 Release zu verhindern
        return;  // Beende loop iteration, normale Note nicht spielen
      }
      
      // Akkord Modus: mehrere Noten gleichzeitig
      if (chordModeType != CHORD_MODE_OFF) {
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
        setLED(i, true);
        setHeldNote(currentNote, i);
      } else {
        // Normal Modus: Note solange gedrückt
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
      
      // LED ausschalten (nur im Normal Modus und nicht im Akkord-Modus)
      if (!holdMode && chordModeType == CHORD_MODE_OFF) {
        setLED(i, false);
      }
      
      // Akkord Modus: Akkord ausschalten (nur wenn NOT in Hold-Modus)
      if (chordModeType != CHORD_MODE_OFF && !holdMode) {
        if (chordModeType == CHORD_MODE_EXTENDED) {
          playChord(i, false);
        } else if (chordModeType == CHORD_MODE_FOLDED) {
          playChordFolded(i, false);
        }
        chordNotesActive[i] = false;
      }
      // Nur im Normal Modus die Note ausschalten
      else if (!holdMode && chordModeType == CHORD_MODE_OFF) {
        noteOn(0x90, currentNote, 0x00);
      }
    }
  }
  
  // Verarbeite Funktions-Schalter (A1-A4)
  handleFunctionSwitches();
}
