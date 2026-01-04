// Hall-Effect Keyboard - 13 Switches
// Digital Pins: 2-12 (ohne 13 wegen LED)
// Analog Pins als Digital: D17 (A3), D18 (A4)

#include "button.h"

const int NUM_SWITCHES = 13;

// Pin-Belegung für alle 13 Switches
const int switchPins[NUM_SWITCHES] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13  // Digital Pins 2-12 (ohne 13)
  18                                 // D17 (A3), D18 (A4) als Digital
};

// Button Objekte für jeden Switch
Button switches[NUM_SWITCHES];

// Speichert ob der Switch gerade gedrückt ist
bool switchPressed[NUM_SWITCHES];

void setup() {
  // Alle Switch-Pins initialisieren
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switches[i].begin(switchPins[i]);
    switchPressed[i] = false;
  }
  
  // Serial Monitor starten
  Serial.begin(9600);
  while (!Serial) {
    ; // Warte auf Serial Connection (wichtig für Leonardo!)
  }
  
  Serial.println("Hall-Effect Keyboard - 13 Switches");
  Serial.println("===================================");
  Serial.println("Pins: D2-D12, D17(A3), D18(A4)");
  Serial.println("Using Button library with bit-shift debouncing");
  Serial.println("Zeigt nur State Changes an");
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

void loop() {
  // Alle Switches durchgehen
  for (int i = 0; i < NUM_SWITCHES; i++) {
    
    // Trigger gibt nur einmal true beim Drücken zurück (wenn Pin LOW wird)
    if (switches[i].trigger()) {
      // Switch wurde gerade gedrückt
      switchPressed[i] = true;
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.println("): PRESSED");
    }
    
    // Prüfe ob Switch losgelassen wurde
    // Pin ist wieder HIGH und war vorher gedrückt
    if (switchPressed[i] && digitalRead(switchPins[i]) == HIGH) {
      // Switch wurde gerade losgelassen
      switchPressed[i] = false;
      Serial.print("Switch ");
      Serial.print(i);
      Serial.print(" (Pin ");
      Serial.print(switchPins[i]);
      Serial.println("): RELEASED");
    }
  }
}
