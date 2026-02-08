#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <EEPROM.h>

/**
 * SETTINGS MANAGER
 * 
 * Verwaltet das Speichern und Laden von Benutzer-Vorgaben im EEPROM.
 * Nutzt die globale Struktur 'KeyboardSettings', um Konsistenz zu gewährleisten.
 */

// Magic Value zur Prüfung der EEPROM-Initialisierung (HALL)
#define SETTINGS_MAGIC 0x48414C4C 

struct KeyboardSettings {
  uint32_t magic;
  uint8_t playModeType;
  int8_t currentOctave;
  int8_t scaleType;
  int8_t chordModeType;
  uint8_t chordExtensionType;
  int8_t diatonicRootKey;
  int8_t arpeggiatorMode;
  uint8_t arpeggiatorRate;
  uint8_t arpeggiatorDutyCycle;
};

// Forward Declarations der globalen Variablen (definiert in den jeweiligen Layer-Files)
extern uint8_t playModeType;
extern int8_t currentOctave;
extern int8_t scaleType;
extern int8_t chordModeType;
extern uint8_t chordExtensionType;
extern int8_t diatonicRootKey;
extern int8_t arpeggiatorMode;
extern uint8_t arpeggiatorRate;
extern uint8_t arpeggiatorDutyCycle;

/**
 * Speichert die aktuellen globalen Variablen ins EEPROM
 */
void saveSettingsToEEPROM() {
  KeyboardSettings settings;
  settings.magic = SETTINGS_MAGIC;
  settings.playModeType = playModeType;
  settings.currentOctave = currentOctave;
  settings.scaleType = scaleType;
  settings.chordModeType = chordModeType;
  settings.chordExtensionType = chordExtensionType;
  settings.diatonicRootKey = diatonicRootKey;
  settings.arpeggiatorMode = arpeggiatorMode;
  settings.arpeggiatorRate = arpeggiatorRate;
  settings.arpeggiatorDutyCycle = arpeggiatorDutyCycle;

  EEPROM.put(0, settings);
  // Serial.println("Settings saved to EEPROM");
}

/**
 * Lädt Einstellungen aus dem EEPROM und weist sie den globalen Variablen zu
 */
void loadSettingsFromEEPROM() {
  KeyboardSettings settings;
  EEPROM.get(0, settings);

  if (settings.magic == SETTINGS_MAGIC) {
    playModeType = settings.playModeType;
    currentOctave = settings.currentOctave;
    scaleType = settings.scaleType;
    chordModeType = settings.chordModeType;
    chordExtensionType = settings.chordExtensionType;
    diatonicRootKey = settings.diatonicRootKey;
    arpeggiatorMode = settings.arpeggiatorMode;
    arpeggiatorRate = settings.arpeggiatorRate;
    arpeggiatorDutyCycle = settings.arpeggiatorDutyCycle;
    // Serial.println("Settings loaded from EEPROM");
  } else {
    // Falls noch nie gespeichert wurde: Initialer Save mit Defaults
    // saveSettingsToEEPROM(); 
    // Serial.println("No valid settings found, using defaults");
  }
}

#endif
