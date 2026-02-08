# Hall-Effect MIDI Keyboard

Ein hochperformanter MIDI-Controller auf Basis von Hall-Effekt-Sensoren und Arduino Leonardo.

## Kurzbeschreibung

Das Keyboard nutzt magnetische Sensoren für präzise Tastenabfragen und bietet fortschrittliche Funktionen für Musiker:

- **13 Hall-Effect Tasten**: Chromatischer Bereich (C-C) mit hoher Auflösung.
- **Intelligente Harmonik**: Echtzeit-Generierung diatonischer Akkorde in verschiedenen Skalen (Major, Minor, Kirchentonleitern etc.).
- **Phasen-Locked Arpeggiator**: Synchronisierte Sequenzen mit einstellbaren Mustern, Beat-Rates und Duty-Cycles.
- **Multifunktionales Interface**: 4 Funktionstasten zur Steuerung von Play-Modi (Hold/Additive) und Systemmenüs.
- **Visuelles Feedback**: Integriertes WS2812 LED-System für Status- und Performance-Anzeige.

## Dokumentation

- Detaillierte Bedienungsanleitung: [docs/MANUAL.md](docs/MANUAL.md)
- Informationen zum Akkord-System: [docs/CHORD_MODE_README.md](docs/CHORD_MODE_README.md)
- Architektur-Details: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

## Technische Features
- **Killall MIDI**: Sauberes Boot-Verhalten.
- **Input Quantization**: Präzises Timing bei Arpeggio-Einstiegen.
- **Low Latency**: Direkte Verarbeitung der Sensorwerte für latenzfreies Spiel.

