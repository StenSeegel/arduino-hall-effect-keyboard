# Hall-Effect Keyboard - Bedienungsanleitung

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
- **Seiten blättern**: In Submenüs mit mehreren Seiten (FS2 & FS3) kann mit einem **Langen Druck auf FS4** zwischen den Unterseiten (Seiten 1-3) geblättert werden.
- **Beenden**: **FS1** (Abbrechen - verwirft Änderungen) oder **FS2** (Speichern & Übernehmen).

---

## Visuelle Rückmeldung (LED-System)

Das LED-System wechselt intelligent zwischen zwei Ebenen:

### 1. Control Layer (Idle / Hauptmenü)
Aktiv, wenn keine Noten gespielt werden oder während der BPM-Priorität (8 Beats nach Tap).
- **LED 0**: FS1 Status (**Rot** - Play Mode)
- **LED 2**: FS2 Status (**Gelb** - Chord Mode)
- **LED 5**: FS3 Status (**Cyan** - Arpeggiator Mode)
- **LED 7**: FS4 Status (**Weiß** - Tap Tempo / Oktav-Anzeige)

### 2. Performance Layer (Note Layer)
Aktiv, sobald Noten gespielt werden oder sich Noten im Arpeggiator/Hold-Speicher befinden.
- **Einfache Note**: Weiß (voll).
- **Mehrfachbelegung (Pooling)**: Wenn mehrere Noten auf einer Taste liegen (z. B. Root + 8th), aber keine spielt: **Gedimmt Weiß**.
- **Chord Root spielt**: **Hell Weiß**.
- **Chord 8th (Oktave) spielt**: **Hell Orange**.
- **Folded Notes**: Werden ebenfalls in **Orange** dargestellt.
- **Inaktivitäts-Delay**: Nach dem Loslassen aller Noten (und leerem Speicher) bleibt das Keyboard noch **250ms** im Performance-Layer.

---

## Spielmodi und Submenüs

### 1. Play Mode (FS1 - Rot)
Dieser Modus steuert, wie Noten gehalten werden.
**Submenü 1 - Optionen**:
- **Index 0**: **Hold <-> Additive** (FS1 wechselt zwischen Single Hold und Additive Hold).
- **Index 1**: **Off <-> Hold** (FS1 schaltet Single Hold ein/aus).
- **Index 2**: **Off <-> Additive** (FS1 schaltet Additive Hold ein/aus).

### 2. Chord Mode (FS2 - Gelb)
Ermöglicht das Spielen von diatonischen Akkorden mit einer Taste.
**Submenü 2 - Seiten**:
- **Seite 1 (Scale Type)**: Major (Ionian), Dorian, Phrygian, Lydian, Mixolydian, Minor (Aeolian), Locrian, Power 5, Power 8.
  - *Grundton*: Drücke eine Note-Taste im Submenü um die Tonart (Root Key) zu definieren.
- **Seite 2 (Chord Layout)**:
  - **Index 0: Stacked** (Normaler Akkordumfang).
  - **Index 1: Folded** (Alle Noten werden in die Basis-Oktave gefaltet).
- **Seite 3 (Chord Extensions)**:
  - **Index 0: Triad** (1-3-5).
  - **Index 1: 7th** (1-3-5-7).
  - **Index 2: 7th + 8th** (1-3-5-7-8 inkl. Oktave).

### 3. Arpeggiator Mode (FS3 - Cyan)
Sequenziert gehaltene Noten rhythmisch.
**Sonderfunktion**: Ein **Long Press auf FS1** im aktiven Arpeggiator-Modus löscht sofort den Arp-Notenspeicher.

**Submenü 3 - Seiten**:
- **Seite 1 (Sequenz-Modus)**: Up/Down, Down/Up, Up, Down, Sequence (nach Anschlagsreihenfolge).
- **Seite 2 (Beat Rate)**: Ganz, Viertel, Achtel, Triolen, Sechzehntel.
- **Seite 3 (Duty Cycle)**: 8 Stufen Artikulation von 10% (Staccato) bis 99% (Legato).

### 4. Oktavierung (FS4 - Weiß)
**Submenü 4 - Optionen**:
- **Oktaven 0-7**: Setzt die MIDI-Oktave (C0 bis C7). Standard ist Oktave 3. 
  - *Vorschau*: Während das Submenü offen ist, können Tasten gedrückt werden, um die neue Oktave Probe zu spielen.

---

## Bedienungsübersicht (Quick Ref)

| Taste | Short Press | Submenu (Long) | Context LED |
|-------|-------------|----------------|-------------|
| **FS1** | Play Mode | Mode Config | Rot |
| **FS2** | Chord Mode | Scale/Root | Gelb |
| **FS3** | Arp Mode | Seq/Rate/Duty | Cyan |
| **FS4** | Tap Tempo | Octave | Weiß |
