// Hall-Effect Sensor Test
// Digitaler Hall-Sensor an Pin 2
// LED und Serial Monitor zeigen Sensor-Status

const int HALL_SENSOR_PIN = 2;  // Hall-Sensor an Digital Pin 2
int lastSensorState = -1;        // Speichert den letzten Sensor-Zustand

void setup() {
  // Pin-Konfiguration
  pinMode(HALL_SENSOR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Serial Monitor starten
  Serial.begin(9600);
  while (!Serial) {
    ; // Warte auf Serial Connection (wichtig für Leonardo!)
  }
  
  Serial.println("Hall-Effect Sensor Test gestartet!");
  Serial.println("Sensor an Pin 2");
  Serial.println("Zeigt nur State Changes an");
  Serial.println("----------------------------------");
}

void loop() {
  // Sensor-Status lesen
  int sensorState = digitalRead(HALL_SENSOR_PIN);
  
  // Nur bei Zustandsänderung ausgeben
  if (sensorState != lastSensorState) {
    if (sensorState == LOW) {
      // Magnet erkannt (Active-Low Sensor)
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Sensor: LOW - Magnet erkannt!");
    } else {
      // Kein Magnet
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Sensor: HIGH - Kein Magnet");
    }
    
    lastSensorState = sensorState;  // Aktuellen Zustand speichern
  }
  
  delay(10);  // Kurze Pause zur Entprellung
}
