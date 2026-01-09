# Hall-Effect Keyboard - Bedienungsanleitung

## Übersicht

Das Hall-Effect Keyboard ist ein MIDI-Controller mit 17 berührungsempfindlichen Tasten, der verschiedene Spielmodi und Akkord-Funktionen bietet. Die intuitive Bedienung ermöglicht sowohl traditionelles Klavierspiel als auch erweiterte Funktionen wie Akkord-Modi und Note-Hold.

## Keyboard Layout

### Haupttastatur (13 Noten-Tasten)
Das Keyboard verfügt über eine **chromatische Oktave** von C bis C, angeordnet wie ein traditionelles Klavier:

**Weiße Tasten** (8 Stück):
- C • D • E • F • G • A • B • C

**Schwarze Tasten** (5 Stück):  
- C# • D# • F# • G# • A#

### Funktionstasten (4 Stück)
Die Funktionstasten verwenden ein **Hauptmenü/Submenu-System**:

#### Hauptmenü (Short Press)
- **FS1**: Play Mode (An/Aus + Submenu-Zugang)
- **FS2**: Akkord Mode (An/Aus + Submenu-Zugang) 
- **FS3**: (Reserviert für Submenu 4)
- **FS4**: (Reserviert für Submenu 4)

#### Submenu-System (Long Press)
**Aktivierung**: Langes Drücken einer Funktionstaste öffnet das entsprechende Submenu
- **Aktuelle Auswahl**: LED entsprechend dem gewählten Index leuchtet in ``Index-Palette`` Farbe
- **Navigation**: FS3/FS4 zum Blättern durch Optionen
- **Beenden**: FS1 (Cancel) oder FS2 (Save & Apply)

### Visuelle Rückmeldung
Das **LED-System** arbeitet mit zwei verschiedenen Modi:

#### Controller Modus Rückmeldung
**Aktiv**: Nur wenn **KEINE** Noten gespielt werden  
**Zweck**: Zeigt aktive Einstellungen und Controller-Status

- **LED 1**: Play Mode Status
- **LED 2**: Chord Mode Status  
- **LEDs 3-8**: Weitere Modi-Anzeigen

**LED-Farben**:
- **Option Index** korrespondiert mit Farbe der ``Index-Palette`` (nur im Controller-Modus)
- **Index-Palette** #ff6961 / #ffb480 / #f8f38d / #42d6a4 / #08cad1 / #59adf6 / #9d94ff / #c780e8 


#### Noten Rückmeldung  
**Aktiv**: Nur wenn **NOTEN** gespielt werden  
**Zweck**: Zeigt welche Noten aktuell klingen

- **LEDs 1-8**: Entsprechende Noten-Tasten (Schwarze Tasten haben alternative Farbe)
- **Wichtig für**: Noten Indikator im Hold-Mode, zeigt gepspielte Noten im Chord-Mode

**LED-Farben**:
- **Weiß / #FFFFFF**: Aktive weiße Tasten
- **Hellblau / #8B8BFF**: Aktive schwarze Tasten  

---

## Spielmodi und Bedienung

### Hauptmenü-Navigation

#### Play Mode (FS1)
**Short Press**: Aktiviert/Deaktiviert aktuellen Play Mode
**Long Press**: Öffnet Submenu 1

**Submenu 1 - Play Mode Optionen:**
- **Index 0** (LED 0): **Hold** - Noten bleiben aktiv nach Loslassen, neue Note führt zum Release der alten
- **Index 1** (LED 1): **Hold + Additive** - Mehrere Noten gleichzeitig halten, Noten können sich nur selbst loslassen (Release)

**LED 1 Status-Anzeige:**
- **Aus**: Kein Modus aktiv
- **Index 0 Farbe**: Hold-Modus aktiv  
- **Index 1 Farbe**: Hold + Additive-Modus aktiv

#### Chord Mode (FS2) 
**Short Press**: Aktiviert/Deaktiviert Chord-Modus
**Long Press**: Öffnet Submenu 2

**Root Note Auswahl:**
- **Note Switches**: Betätigung eines Note Switches setzt neue Root Note (Zugehörige LED blinkt 3x 300ms zur Bestätigung - Standard für Note-Tasten Settings)

**Scale Type Auswahl:**
- **Index 0** (LED 0 in Index-Palette Farbe): **Major** - Tasten spielen diatonische Akkorde für Major Scale, nicht-diatonische Töne werden deaktiviert
- **Index 1** (LED 1 in Index-Palette Farbe): **Dorian** - Diatonische Akkorde für Dorian Modus
- **Index 2** (LED 2 in Index-Palette Farbe): **Phrygian** - Diatonische Akkorde für Phrygian Modus  
- **Index 3** (LED 3 in Index-Palette Farbe): **Lydian** - Diatonische Akkorde für Lydian Modus
- **Index 4** (LED 4 in Index-Palette Farbe): **Mixolydian** - Diatonische Akkorde für Mixolydian Modus
- **Index 5** (LED 5 in Index-Palette Farbe): **Minor** - Tasten spielen diatonische Akkorde für Minor Scale, nicht-diatonische Töne werden deaktiviert
- **Index 6** (LED 6 in Index-Palette Farbe): **Locrian** - Diatonische Akkorde für Locrian Modus
- **Index 7** (LED 7 in Index-Palette Farbe): **Power 5** - Alle Tasten spielen Power 5 Akkorde
- **Index 8** (LED 8 in Index-Palette Farbe): **Power 8** - Alle Tasten spielen Power 8 Akkorde

**LED 2 Status-Anzeige:**
- **Aus**: Kein Chord Mode aktiv
- **Index 0 Farbe**: Major Scale aktiv
- **Index 1 Farbe**: Dorian Mode aktiv
- **Index 2 Farbe**: Phrygian Mode aktiv
- **Index 3 Farbe**: Lydian Mode aktiv
- **Index 4 Farbe**: Mixolydian Mode aktiv
- **Index 5 Farbe**: Minor Scale aktiv
- **Index 6 Farbe**: Locrian Mode aktiv
- **Index 7 Farbe**: Power 5 Scale aktiv
- **Index 8 Farbe**: Power 8 Scale aktiv

#### Oktavierung (FS4)
**Long Press**: Öffnet Submenu 4 (Oktavumschaltung)

**Note Preview:**
- **Note Switches**: Betätigung eines Note Switches spielt Noten in der gewählten Oktave, ohne das Submenu verlassen zu müssen.
- **Cancel**: Kehrt zur zuletzt aktiven Oktave vor dem Öffnen des Submenüs zurück.

**Submenu 4 - Oktavierungs-Optionen:**
- **Index 0** (LED 0): **Oktave 0** - Tiefste Oktave (MIDI C0 = Note 0)
- **Index 1** (LED 1): **Oktave 1** 
- **Index 2** (LED 2): **Oktave 2**
- **Index 3** (LED 3): **Oktave 3** - Standardoktave beim Start
- **Index 4** (LED 4): **Oktave 4** 
- **Index 5** (LED 5): **Oktave 5** 
- **Index 6** (LED 6): **Oktave 6** 
- **Index 7** (LED 7): **Oktave 7** - Höchste Oktave (MIDI C7 = Note 84)

### Submenu-Navigation
**In jedem Submenu:**
- **FS3**: Vorherige Option (Index runter)
- **FS4**: Nächste Option (Index hoch)  
- **FS1**: Cancel (ohne Speichern zurück zum Hauptmenü)
- **FS2**: Save & Apply (Speichert Auswahl und kehrt zurück)

**Visuelle Anzeigen:**
- **LED**: Zeigt aktuell gewählten Index in ``Index-Palette`` Farbe

### Standard-Spielmodi

#### OFF-Modus (Standard)
**Bedienung**: Normale Klaviatur-Funktion
- Taste drücken → Note erklingt
- Taste halten → Note spielt weiter  
- Taste loslassen → Note stoppt

#### Hold-Modus
**Bedienung**: Noten bleiben aktiv ohne Taste zu halten
- Taste einmal drücken → Note startet und hält
- Dieselbe Taste erneut drücken → Note stoppt
- Andere Taste drücken → Vorherige Note stoppt, neue startet

#### Hold + Additive-Modus
**Bedienung**: Mehrere Noten gleichzeitig halten
- Jede Taste funktioniert als Ein/Aus-Schalter
- Mehrere Noten können parallel aktiv sein
- Erneutes Drücken schaltet jeweilige Note aus

### Chord-Modus  
**Funktion**: Jede Taste spielt einen vollständigen Akkord (3 Noten)

**Akkord-System:**
Das System generiert automatisch diatonisch korrekte Akkorde basierend auf:
- **Root Note** (Grundtonart) - gesteuert über Note Switches im Submenu 2
- **Scale Type** (Skalentyp) - gesteuert über Submenu 2 Index-Optionen

#### Scale Types (Submenu 2 Index-Optionen)
- **Index 0**: **Ionian (Major)** - Diatonische Akkorde basierend auf Major Scale Pattern
- **Index 1**: **Dorian** - Diatonische Akkorde basierend auf Dorian Mode Pattern
- **Index 2**: **Phrygian** - Diatonische Akkorde basierend auf Phrygian Mode Pattern  
- **Index 3**: **Lydian** - Diatonische Akkorde basierend auf Lydian Mode Pattern
- **Index 4**: **Mixolydian** - Diatonische Akkorde basierend auf Mixolydian Mode Pattern
- **Index 5**: **Aeolian (Minor)** - Diatonische Akkorde basierend auf Minor Scale Pattern
- **Index 6**: **Locrian** - Diatonische Akkorde basierend auf Locrian Mode Pattern
- **Index 7**: **Power 5** - Alle Tasten spielen Power 5 Akkorde
- **Index 8**: **Power 8** - Alle Tasten spielen Power 8 Akkorde

#### Diatonische Muster

**Major Scale Pattern** (I-VII):
Major → Minor → Minor → Major → Major → Minor → Diminished

**Minor Scale Pattern** (i-vii):  
Minor → Diminished → Major → Minor → Minor → Major → Major

**Beispiel C-Dur** (Root Note = C):
- **C**: Major (C-E-G)
- **D**: Minor (D-F-A)
- **E**: Minor (E-G-B)  
- **F**: Major (F-A-C)
- **G**: Major (G-B-D)
- **A**: Minor (A-C-E)
- **B**: Diminished (B-D-F)

**Beispiel a-moll** (Root Note = A, Minor Scale):
- **A**: Minor (A-C-E)
- **B**: Diminished (B-D-F)
- **C**: Major (C-E-G)
- **D**: Minor (D-F-A)
- **E**: Minor (E-G-B)
- **F**: Major (F-A-C)
- **G**: Major (G-B-D)

## Bedienungsübersicht

### Hauptmenü-Funktionen
| Taste | Short Press | Long Press | LED-Status |
|-------|-------------|------------|------------|
| **FS1** | Play Mode Ein/Aus | Submenu 1 öffnen | LED 1: aus oder ``Index-Palette`` |
| **FS2** | Akkord Mode Ein/Aus | Submenu 2 öffnen | LED 2: aus oder ``Index-Palette`` |
| **FS3** | (Reserviert) | Submenu 4 öffnen | LED zeigt aktuellen Index |
| **FS4** | (Reserviert) | Submenu 4 öffnen | LED zeigt aktuellen Index |

### Submenu-Navigation
| Taste | Funktion | Beschreibung |
|-------|----------|-------------|
| **FS1** | Cancel | Zurück ohne Speichern |
| **FS2** | Save & Apply | Speichern und zurück |
| **FS3** | Index runter | Vorherige Option |
| **FS4** | Index hoch | Nächste Option |

---

## Submenu-Übersicht

### Submenu 1 - Play Mode
**Optionen**: Hold (Index 0) | Hold + Additive (Index 1)
- Wähle Play Mode: Navigation mit FS3/FS4
- Speichere: FS2 (Save & Apply)

### Submenu 2 - Chord Mode / Scale Type
**Optionen**: Major | Dorian | Phrygian | Lydian | Mixolydian | Minor | Locrian | Power 5 | Power 8
- Wähle Scale Type: Navigation mit FS3/FS4
- Setze Root Note: Betätige eine Note-Taste (LED bestätigt 3x Blinken)
- Speichere: FS2 (Save & Apply)

### Submenu 4 - Octave Selection
**Optionen**: Oktave 0-7
- Wähle Oktave: Navigation mit FS3/FS4
- Speichere: FS2 (Save & Apply)

---

## Anwendungsszenarien

### Live Performance
- **OFF-Modus**: Für traditionelles Klavierspiel
- **Hold-Modus**: Für Basslinien oder gehaltene Noten  
- **Hold+Additive**: Für komplexe Arrangements mit mehreren gehaltenen Noten
- **Akkord-Modus**: Für schnelle Begleitung

### Songwriting
- **Akkord-Modus**: Zum Testen von Harmoniefolgen  
- **Hold+Additive**: Für Experimente mit Klangkombinationen
- **Oktavierung**: Für verschiedene Tonlagen

### Jamming  
- **Power-Chords**: Für Rock/Pop Styles
- **Diatonische Akkorde**: Für Jazz/Folk Harmonien
- **Hold-Modi**: Für spontane Loop-ähnliche Effekte