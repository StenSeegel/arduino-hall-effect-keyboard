// Hall-Effect Keyboard - 13 Switches
// 

#include "button.h"
#include "arduino_stubs.h"

const int NUM_SWITCHES = 13;

// Pin-Belegung für alle 13 Switches
const int switchPins[NUM_SWITCHES] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  // Digital Pins 2-12 (ohne 13)
  18                                 // D17 (A3), D18 (A4) als Digital
};

// MIDI Noten für jeden Switch (C bis C, relativ zu Oktave 0)
const int midiNotes[NUM_SWITCHES] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12  // C, C#, D, D#, E, F, F#, G, G#, A, A#, B, C
};

// Oktav-Einstellung (Standard: 3, entspricht C3)
int currentOctave = 3;

// Button Objekte für jeden Switch
Button switches[NUM_SWITCHES];

// Speichert ob der Switch gerade gedrückt ist
bool switchPressed[NUM_SWITCHES];

// Hold-Schalter Variables
bool holdMode = false;               // Toggle zwischen Normal und Hold Modus
bool additiveMode = false;          // Additive Hold: mehrere Noten gleichzeitig
int heldNote = -1;                  // Aktuell gehaltene Note (-1 = keine) - nur für nicht-additiv
bool heldNotes[NUM_SWITCHES];       // Array für additive Noten

// Widerstandsleiter Objekt (4 Schalter an A1)
LadderSwitch ladderSwitch;

void setup() {
  // Alle Switch-Pins initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switches[i].begin(switchPins[i]);
    switchPressed[i] = false;
    heldNotes[i] = false;
  }
  
  // Widerstandsleiter initialisieren
  ladderSwitch.begin(A1);
  
  // Serial Monitor starten
  Serial.begin(9600);
  // Midi Serial über TX1/RX1
  Serial1.begin(31250);
  while (!Serial) {
    ; // Warte auf Serial Connection (wichtig für Leonardo!)
  }
  
  // Analog Pin für Widerstandsleiter initialisieren
  pinMode(A1, INPUT);
  
  Serial.println("Hall-Effect Keyboard - 13 Switches");
  Serial.println("===================================");
  Serial.println("Pins: D2-D12, D17(A3), D18(A4)");
  Serial.println("Using Button library with bit-shift debouncing");
  Serial.println("Zeigt nur State Changes an");
  Serial.println("4 Ladder Switches an A1");
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
}

// MIDI Note On/Off Funktion
void noteOn(int cmd, int pitch, int velocity) {
  Serial1.write(cmd);
  Serial1.write(pitch);
  Serial1.write(velocity);
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

void handleLadderSwitch() {
  int switchIndex = ladderSwitch.getSwitch();
  
  // Nur verarbeiten wenn sich der Schalter geändert hat (getSwitch() gibt -1 zurück wenn keine Änderung)
  if (switchIndex >= 0) {
    Serial.print("Ladder Switch ");
    Serial.print(switchIndex + 1);
    Serial.println(" PRESSED");
    
    // Hier können Funktionen für die 4 Schalter aufgerufen werden
    // z.B. toggleHoldMode(), toggleAdditiveMode(), etc.
    switch(switchIndex) {
      case 0:  // Schalter 1
        // toggleHoldMode();
        break;
      case 1:  // Schalter 2
        // toggleAdditiveMode();
        break;
      case 2:  // Schalter 3
        // incrementOctave();
        break;
      case 3:  // Schalter 4
        // decrementOctave();
        break;
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
    heldNote = -1;
  } else if (additiveMode) {
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (heldNotes[i]) {
        int note = midiNotes[i] + (currentOctave * 12);
        noteOn(0x90, note, 0x00);
        heldNotes[i] = false;
      }
    }
  }
  
  additiveMode = !additiveMode;
  Serial.print("Additive Modus: ");
  Serial.println(additiveMode ? "ON" : "OFF");
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
      heldNotes[switchIndex] = false;
    } else {
      // Note anschalten
      noteOn(0x90, note, 0x45);
      Serial.print("Hold Note On (Additive): ");
      Serial.println(note);
      heldNotes[switchIndex] = true;
    }
  } else {
    // Standard Hold Modus: eine Note nach der anderen
    if (heldNote != -1 && heldNote != note) {
      // Alte Note ausschalten
      noteOn(0x90, heldNote, 0x00);
      Serial.print("Hold Note Off: ");
      Serial.println(heldNote);
    }
    
    // Neue Note anschalten oder deaktivieren
    if (note == heldNote) {
      // Toggle - gleiche Note wurde erneut gedrückt
      heldNote = -1;
    } else {
      // Neue Note anschalten
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
      switchPressed[i] = true;
      int currentNote = midiNotes[i] + (currentOctave * 12);
      
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.print("): PRESSED - Note ");
      Serial.println(currentNote);
      
      // Hold Modus: Note halten bis zum nächsten Drücken
      if (holdMode) {
        setHeldNote(currentNote, i);
      } else {
        // Normal Modus: Note solange gedrückt
        noteOn(0x90, currentNote, 0x45);
      }
    }
    
    // Prüfe ob Switch losgelassen wurde
    // Pin ist wieder HIGH und war vorher gedrückt
    if (switchPressed[i] && digitalRead(switchPins[i]) == HIGH) {
      // Switch wurde gerade losgelassen
      switchPressed[i] = false;
      int currentNote = midiNotes[i] + (currentOctave * 12);
      
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.print("): RELEASED - Note ");
      Serial.println(currentNote);
      
      // Nur im Normal Modus die Note ausschalten
      if (!holdMode) {
        noteOn(0x90, currentNote, 0x00);
      }
    }
  }
  
  // Verarbeite Widerstandsleiter-Schalter
  handleLadderSwitch();
}
