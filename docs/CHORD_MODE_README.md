# Akkord-Modus für Hall-Effect Keyboard

## Übersicht

Der Akkord-Modus ermöglicht es dir, mit einer einzelnen Tasten-Betätigung einen vordefinierten diatonischen Akkord zu triggern. Die Akkorde basieren auf der **C-Dur-Tonleiter** und folgen klassischen Akkordstrukturen.

## Aktivierung

Drücke **Funktionsschalter 2 (A2)** um den Akkord-Modus ein/auszuschalten.
- Serieller Monitor zeigt: `Akkord Modus: ON` oder `Akkord Modus: OFF`

## Akkord-Definition (Diatonisch in C-Dur)

Die Akkorde folgen den Stufen der C-Dur-Tonleiter. Standardmäßig spielt jede Taste einen Dreiklang bestehend aus Grundnote, Terz und Quinte:

| Taste | Stufe | Akkordtyp | Akkord    | Noten       |
|-------|-------|-----------|-----------|-------------|
| **C** | I     | Major     | C Major   | C - E - G   |
| C#    | —     | Power 5   | C# 5      | C# - G# - C#|
| **D** | ii    | Minor     | D minor   | D - F - A   |
| D#    | —     | Power 5   | D# 5      | D# - A# - D#|
| **E** | iii   | Minor     | E minor   | E - G - B   |
| **F** | IV    | Major     | F Major   | F - A - C   |
| F#    | —     | Power 5   | F# 5      | F# - C# - F#|
| **G** | V     | Major     | G Major   | G - B - D   |
| G#    | —     | Power 5   | G# 5      | G# - D# - G#|
| **A** | vi    | Minor     | A minor   | A - C - E   |
| A#    | —     | Power 5   | A# 5      | A# - F - A# |
| **B** | vii°  | Diminished| B dim     | B - D - F   |
| **C** | I     | Major     | C Major   | C - E - G   |

**Weiße Tasten** (fett) sind die diatonischen Akkorde in C-Dur.
**Schwarze Tasten** spielen Power Chords (5er-Akkorde ohne Terz).

## Nutzung

1. Drücke **A2** um Akkord-Modus zu aktivieren (serieller Monitor bestätigt dies)
2. **Taste beliebig drücken** → Der komplette diatonische Akkord wird gespielt
3. **Taste halten** → Akkord erklingt, solange die Taste gedrückt ist
4. **Taste loslassen** → Akkord verstummt

## Chord Extensions (Submenu 2, Seite 3)

Im **FS2-Submenu** (Chord Mode) kann auf Seite 3 der Umfang des Akkords gewählt werden:

- **Index 0: Triad Chords** → Dreiklänge (1-3-5)
- **Index 1: 7th Chords** → Vierklänge (1-3-5-7)
- **Index 2: 7th + 8ths Chords** → Fünfklänge inkl. Oktave (1-3-5-7-8)

Die zusätzlichen Töne werden **diatonisch** basierend auf dem gewählten Modus (Submenu 2, Seite 1) berechnet.

## LED-Visualisierung im Akkord/Arp-Modus

Wenn mehrere Noten auf einer LED liegen (z. B. Root und Oktave), wird der Status farblich priorisiert dargestellt:

- **Hell-Weiß (Bright White)**: Die Grundnote (Root) des Akkords spielt gerade.
- **Hell-Orange (Bright Orange)**: Die 8. Note (Oktave) des Akkords spielt gerade.
- **Gedimmt-Weiß (Dimmed White)**: Die Noten liegen im Arpeggiator-Pool, werden aber gerade nicht aktiv gespielt.

Es findet **kein Blinken** statt, um eine ruhige und eindeutige Anzeige des aktuell klingenden Tons zu gewährleisten.

## Anpassung der Akkorde

Um die Akkorde anzupassen, modifiziere die diatonische Logik in `ChordMode.h`:

```cpp
// Siehe getDiatonicChordNote() und chordDefinitions[]
```

### Akkordtypen (Semitone-Intervalle):

**Major Akkord:** `{0, 4, 7}`
- Grundnote + große Terz (4 Semitone) + Quinte (7 Semitone)
- Beispiel: C Major = C (0) + E (4) + G (7)

**Minor Akkord:** `{0, 3, 7}`
- Grundnote + kleine Terz (3 Semitone) + Quinte (7 Semitone)
- Beispiel: D minor = D (0) + F (3) + A (7)

**Diminished Akkord:** `{0, 3, 6}`
- Grundnote + kleine Terz (3 Semitone) + kleine Quinte (6 Semitone)
- Beispiel: B diminished = B (0) + D (3) + F (6)

**Power Chord:** `{0, 7, 12}`
- Grundnote + Quinte + Oktave
- Kraftvoll, aber weniger harmonisch

### Weitere Akkordtypen:

- **Augmented (aug):** `{0, 4, 8}` — Grundnote + große Terz + übermäßige Quinte
- **Suspended 4 (sus4):** `{0, 5, 7}` — Grundnote + Quarte + Quinte
- **Dominant 7 (7):** `{0, 4, 7, 10}` — Major + kleine Septime (benötigt maxChordNotes = 4)

## Semitone-Referenztabelle

| Semitone | Intervall | Beispiel von C |
|----------|-----------|---|
| 0 | Grundnote (Unison) | C |
| 1 | Kleine Sekunde | C# |
| 2 | Große Sekunde | D |
| 3 | Kleine Terz | Eb |
| 4 | Große Terz | E |
| 5 | Quarte | F |
| 6 | Tritonus (kleine Quinte) | F# |
| 7 | Quinte (Perfect Fifth) | G |
| 8 | Kleine Sexte | G# |
| 9 | Große Sexte | A |
| 10 | Kleine Septime | Bb |
| 11 | Große Septime | B |
| 12 | Oktave | C (nächste Oktave) |

## Kompatibilität

- Der Akkord-Modus ist **unabhängig** von Hold- und Additiv-Modus
- Er kann alleine verwendet werden
- Im Akkord-Modus wird jede Taste-Betätigung als Akkord-Trigger interpretiert

## Serielle Ausgabe

Wenn Akkorde gespielt werden, zeigt die serielle Konsole z.B.:
```
Akkord Modus: ON
Chord Note On: 60
Chord Note On: 64
Chord Note On: 67
Switch 0 (Pin 2): PRESSED - Note 60
...
Chord Note Off: 60
Chord Note Off: 64
Chord Note Off: 67
Switch 0 (Pin 2): RELEASED - Note 60
```

