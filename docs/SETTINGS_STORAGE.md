# EEPROM Settings Storage

Diese Tabelle beschreibt die im EEPROM gespeicherten Parameter des Hall-Effect Keyboards.

| Parameter | Typ | Beschreibung | Standardwert |
| :--- | :--- | :--- | :--- |
| `magic` | uint32 | Magic Value zur Initialisierungsprüfung (`0x48414C4C`) | - |
| `playModeType` | uint8 | Verhalten von FS1 (Hold/Add/Hold+Add) | 0 |
| `currentOctave` | int8 | Aktuelle MIDI-Oktave | 3 |
| `scaleType` | int8 | Chord Mode: Gewählte Skala (0-8) | 0 (Ionian) |
| `chordModeType` | int8 | Chord Mode: Typ (0=Off, 1=Extended, 2=Folded) | 0 |
| `chordExtensionType` | uint8 | Chord Mode: Erweiterung (Triad, 7th...) | 0 |
| `diatonicRootKey` | int8 | Chord Mode: Grundton (0-11) | 0 (C) |
| `arpeggiatorMode` | int8 | Arp: Abspielmuster | 0 (Up/Down) |
| `arpeggiatorRate` | uint8 | Arp: Geschwindigkeit (1/4, 1/8...) | 2 (1/8) |
| `arpeggiatorDutyCycle` | uint8 | Arp: Gate-Zeit in % | 50 |

Die Einstellungen werden automatisch beim Systemstart aus dem EEPROM geladen und bei jeder Parameteränderung in einem Submenü (Bestätigung mit FS2) gespeichert.
