# Hall-Effect Keyboard - Bedienungsanleitung

## Übersicht

Das Hall-Effect Keyboard ist ein MIDI-Controller mit 17 berührungsempfindlichen Tasten (Hall-Sensoren), der verschiedene Spielmodi, einen intelligenten Akkord-Modus und einen rhythmus-synchronisierten Arpeggiator bietet. Das System läuft auf einem Arduino Leonardo.

## Keyboard Layout

### Haupttastatur (13 Noten-Tasten)
Das Keyboard verfügt über eine **chromatische Oktave** (C bis C). Die Hall-Sensoren ermöglichen ein präzises Spielgefühl.

**Weiße Tasten** (8 Stück): C • D • E • F • G • A • B • C
**Schwarze Tasten** (5 Stück): C# • D# • F# • G# • A#

### Funktionstasten (4 Stück)
Die Funktionstasten (FS1 bis FS4) steuern das System über ein **Hauptmenü/Submenü-System**.

#### Hauptmenü (Short Press)
- **FS1**: **Play Mode** (An/Aus) - Steuert Hold-Funktionen.
- **FS2**: **Chord Mode** (An/Aus) - Aktiviert intelligente Akkorde.
- **FS3**: **Arpeggiator Mode** (An/Aus) - Aktiviert den rhythmischen Arpeggiator.
- **FS4**: **Tap Tempo** - Den Button rhythmisch drücken, um das Tempo (BPM) zu setzen (min. 3 Taps erforderlich).

#### Submenü-System (Long Press)
**Aktivierung**: Ein langer Druck auf eine Funktionstaste öffnet das zugehörige Submenü.
- **Navigation**: **FS3** (Index runter) und **FS4** (Index hoch).
- **Seiten blättern**: In Submenüs mit mehreren Seiten (2 & 3) kann mit einem **Langen Druck auf FS4** zwischen den Unterseiten geblättert werden.
- **Beenden**: **FS1** (Abbrechen) oder **FS2** (Speichern & Übernehmen).

---

## Visuelle Rückmeldung (LED-System)

Das LED-System wechselt intelligent zwischen zwei Ebenen:

### 1. Control Layer (Idle / Hauptmenü)
Aktiv, wenn keine Noten gespielt werden oder während der BPM-Priorität (8 Beats nach Tap).
- **LED 0**: Play Mode Status (**Blau**)
- **LED 2**: Chord Mode Status (**Gelb**)
- **LED 5**: Arpeggiator Mode Status (**Cyan**)
- **LED 7**: Tap Tempo Indikator (**Weiß**, blinkt phasen-synchron zum Arpeggiator)

### 2. Performance Layer (Note Layer)
Aktiv, sobald Noten gespielt werden oder sich Noten im Arpeggiator/Hold-Speicher befinden.
- **Aktive Noten**: Leuchten hell (Weiß für Stammtöne, Pink für chromatische Töne).
- **Arp-Memory-Notes**: Im Arpeggiator-Modus leuchten registrierte, aber gerade nicht spielte Noten schwach, die aktuell gespielte Note leuchtet hell.
- **Inaktivitäts-Delay**: Nach dem Loslassen aller Noten (und leerem Arp-Speicher) bleibt das Keyboard noch **250ms** im Performance-Layer. Ein Druck auf eine FS-Taste wechselt sofort zurück.

---

## Spielmodi und Submenüs

### 1. Play Mode (FS1 - Blau)
**Submenü 1 Hintergrund**: Blau
- **Index 0**: **Single Hold** - Die zuletzt gedrückte Note (oder Akkord) wird gehalten.
- **Index 1**: **Additive Hold** - Jede Note fungiert als Toggle (Ein/Aus). Mehrere Noten können gleichzeitig gehalten werden.

### 2. Chord Mode (FS2 - Gelb)
**Submenü 2 - Seite 1 (Skalentyp)**:
- **Skalentyp (0-8)**: Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Locrian, Power 5, Power 8.
- **Grundton setzen**: Drücke eine Note-Taste im Submenü, um die Tonart (Root Key) zu definieren.

**Submenü 2 - Seite 2 (Akkord-Typ)**:
- **Index 0**: **Stacked** (Normaler Akkord)
- **Index 1**: **Folded** (Akkord-Noten werden in eine Oktave gefaltet)

### 3. Arpeggiator Mode (FS3 - Cyan)
Der Arpeggiator ist phasen-synchron zum Tap Tempo (Phasen-Lock).

**Submenü 3 - Seite 1 (Sequenz-Modus)**:
- **Mode 0**: Up/Down
- **Mode 1**: Down/Up
- **Mode 2**: Up
- **Mode 3**: Down
- **Mode 4**: **Sequence** (Spielt die Noten in der Reihenfolge des Drückens).
  - *Sonderfall*: Aktiviert automatisch *Additive Hold*, wenn beim Start kein Hold/Chord aktiv ist.

**Submenü 3 - Seite 2 (Beat Rate)**:
- **Optionen**: Ganz, Viertel, Achtel, Triolen, Sechzehntel.

**Submenü 3 - Seite 3 (Duty Cycle)**:
*Blättern mit Long Press auf FS4*
- **Optionen**: 8 Stufen (10%, 25%, 40%, 50%, 60%, 75%, 90%, 99%).
- **Echtzeit-Vorschau**: Die Artikulation (Staccato bis Legato) ist sofort hörbar.

### 4. Oktavierung (FS4 - Weiß)
**Long Press**: Öffnet Submenü 4.
**Submenü 4 Hintergrund**: Weiß
- **Oktaven 0-7**: Setzt die Basis-Oktave des Keyboards (MIDI C0 bis C7). Standard ist Oktave 3.

---

## Technische Highlights
- **Killall MIDI**: Sendet beim Bootup "All Notes Off" und explizite Note-Offs für alle 128 Kanäle.
- **Phasen-Lock Sync**: Arpeggiator und LEDs nutzen einen gemeinsamen Progress-Timer (kein Auseinanderlaufen möglich).
- **Eingangs-Quantisierung**: Arpeggiator-Noten starten immer exakt auf dem nächsten rhythmischen Teilungspunkt.
- **Submenü Persistence**: Der Arpeggiator läuft im Hintergrund weiter, während Oktaven oder Skalen im Menü angepasst werden.

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

#### Arpeggiator (FS3)
**Short Press**: Aktiviert/Deaktiviert Arp-Modus
**Long Press**: Öffnet Submenu 3 (Arpeggiator)

**Arpeggiator Preview:**
- **Note Switches**: Betätigung eines Note Switches spielt Noten in der gewählten Oktave, ohne das Submenu verlassen zu müssen.
- **Cancel**: Kehrt zur zuletzt aktiven Einstellung vor dem Öffnen des Submenüs zurück.

**Arpeggiator Funktionen:**
- **Generelle Funktionalität**: Spielt gehaltene Noten automatisch nacheinander in verschiedenen Mustern
- **Hold-Integration**: Funktioniert mit Hold- und Hold+Additive Modi
- **Oktav-Integration**: Verwendet aktuelle Oktavierung aus Submenu 4

**Submenu 3 - Arpeggiator-Optionen:**
- **Index 0** (LED 0): **Up Down** - Spielt von tiefster zur höchsten Note und zurück, ohne Noten zu wiederholen.
- **Index 1** (LED 1): **Down Up** - Spielt von höchster zur tiefsten Note und zurück, ohne Noten zu wiederholen.
- **Index 2** (LED 2): **Up** - Spielt von tiefster zur höchsten Note und beginnt wieder von der tiefsten
- **Index 3** (LED 3): **Down** - Spielt von höchster zur tiefsten Note und beginnt wieder von der höchsten

**LED 3 Status-Anzeige:**
- **Aus**: Kein Arpeggiator aktiv
- **Index 0 Farbe**: Up Down Modus aktiv  
- **Index 1 Farbe**: Down Up Modus aktiv
- **Index 2 Farbe**: Up Modus aktiv
- **Index 3 Farbe**: Down Modus aktiv

**LED 8 Tempo-Anzeige (Tap Tempo):**
- **Aus**: Arpeggiator inaktiv
- **Blinkt**: Arpeggiator aktiv - LED blinkt im Tempo des Arpeggiators
  - Schnelles Blinken = schnelles Tempo
  - Langsames Blinken = langsames Tempo


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
| **FS3** | Arpeggiator Ein/Aus | Submenu 3 öffnen | LED 3: aus oder ``Index-Palette`` |
| **FS4** | Tap Tempo (global) | Submenu 4 öffnen | LED 8: blinkt wenn Arpeggiator aktiv |

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