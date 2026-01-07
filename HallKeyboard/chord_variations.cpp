// AKKORD-VARIATIONEN FÜR HALLKEYBOARD
// Diese Datei enthält alternative Akkorddefinitionen, die du verwenden kannst
// Kopiere die gewünschte Definition in HallKeyboard.ino und ersetze das dortige "chords" Array

// ===== ORIGINAL: MAJOR AKKORDE =====
// Alle Tasten spielen Major-Akkorde (Standard)
const int chords_major[13][3] = {
  {0, 4, 7},   // C Major:  C - E - G
  {0, 4, 7},   // C# Major: C# - F - G#
  {0, 4, 7},   // D Major:  D - F# - A
  {0, 4, 7},   // D# Major: D# - G - A#
  {0, 4, 7},   // E Major:  E - G# - B
  {0, 4, 7},   // F Major:  F - A - C
  {0, 4, 7},   // F# Major: F# - A# - C#
  {0, 4, 7},   // G Major:  G - B - D
  {0, 4, 7},   // G# Major: G# - C - D#
  {0, 4, 7},   // A Major:  A - C# - E
  {0, 4, 7},   // A# Major: A# - D - F
  {0, 4, 7},   // B Major:  B - D# - F#
  {0, 4, 7}    // C Major:  C - E - G
};

// ===== MINOR AKKORDE =====
// Alle Tasten spielen Minor-Akkorde (kleine Terz statt großer Terz)
const int chords_minor[13][3] = {
  {0, 3, 7},   // C Minor:  C - Eb - G
  {0, 3, 7},   // C# Minor: C# - E - G#
  {0, 3, 7},   // D Minor:  D - F - A
  {0, 3, 7},   // D# Minor: D# - F# - A#
  {0, 3, 7},   // E Minor:  E - G - B
  {0, 3, 7},   // F Minor:  F - Ab - C
  {0, 3, 7},   // F# Minor: F# - A - C#
  {0, 3, 7},   // G Minor:  G - Bb - D
  {0, 3, 7},   // G# Minor: G# - B - D#
  {0, 3, 7},   // A Minor:  A - C - E
  {0, 3, 7},   // A# Minor: A# - C# - F
  {0, 3, 7},   // B Minor:  B - D - F#
  {0, 3, 7}    // C Minor:  C - Eb - G
};

// ===== POWER CHORDS (Quint-Akkorde) =====
// Nur Grundnote und Quinte, plus Oktave (klangvoll und kraftvoll)
// Nicht ideal für MIDI, aber möglich
const int chords_power[13][3] = {
  {0, 7, 12},   // C Power: C - G - C(Oktave)
  {0, 7, 12},   // C# Power: C# - G# - C#
  {0, 7, 12},   // D Power: D - A - D
  {0, 7, 12},   // D# Power: D# - A# - D#
  {0, 7, 12},   // E Power: E - B - E
  {0, 7, 12},   // F Power: F - C - F
  {0, 7, 12},   // F# Power: F# - C# - F#
  {0, 7, 12},   // G Power: G - D - G
  {0, 7, 12},   // G# Power: G# - D# - G#
  {0, 7, 12},   // A Power: A - E - A
  {0, 7, 12},   // A# Power: A# - F - A#
  {0, 7, 12},   // B Power: B - F# - B
  {0, 7, 12}    // C Power: C - G - C
};

// ===== DOMINANT 7 AKKORDE (4 Noten) =====
// WARNUNG: Diese benötigen 4 Positionen im Array statt 3
// Erfordert maxChordNotes = 4 in HallKeyboard.ino
const int chords_dom7[13][4] = {
  {0, 4, 7, 10},   // C7: C - E - G - Bb
  {0, 4, 7, 10},   // C#7: C# - F - G# - B
  {0, 4, 7, 10},   // D7: D - F# - A - C
  {0, 4, 7, 10},   // D#7: D# - G - A# - C#
  {0, 4, 7, 10},   // E7: E - G# - B - D
  {0, 4, 7, 10},   // F7: F - A - C - Eb
  {0, 4, 7, 10},   // F#7: F# - A# - C# - E
  {0, 4, 7, 10},   // G7: G - B - D - F
  {0, 4, 7, 10},   // G#7: G# - C - D# - F#
  {0, 4, 7, 10},   // A7: A - C# - E - G
  {0, 4, 7, 10},   // A#7: A# - D - F - G#
  {0, 4, 7, 10},   // B7: B - D# - F# - A
  {0, 4, 7, 10}    // C7: C - E - G - Bb
};

// ===== SUSPENED 4 AKKORDE (sus4) =====
// Quarte statt Terz - offener, schwebender Sound
const int chords_sus4[13][3] = {
  {0, 5, 7},   // Csus4: C - F - G
  {0, 5, 7},   // C#sus4: C# - F# - G#
  {0, 5, 7},   // Dsus4: D - G - A
  {0, 5, 7},   // D#sus4: D# - G# - A#
  {0, 5, 7},   // Esus4: E - A - B
  {0, 5, 7},   // Fsus4: F - Bb - C
  {0, 5, 7},   // F#sus4: F# - B - C#
  {0, 5, 7},   // Gsus4: G - C - D
  {0, 5, 7},   // G#sus4: G# - C# - D#
  {0, 5, 7},   // Asus4: A - D - E
  {0, 5, 7},   // A#sus4: A# - D# - F
  {0, 5, 7},   // Bsus4: B - E - F#
  {0, 5, 7}    // Csus4: C - F - G
};

// ===== AUGMENTED AKKORDE (Übermäßig) =====
// Erhöhte Quinte - dramatischer, spannungsvoller Sound
const int chords_aug[13][3] = {
  {0, 4, 8},   // Caug: C - E - G#
  {0, 4, 8},   // C#aug: C# - E# - G##(A)
  {0, 4, 8},   // Daug: D - F# - A#
  {0, 4, 8},   // D#aug: D# - F##(G) - A##(B)
  {0, 4, 8},   // Eaug: E - G# - B#(C)
  {0, 4, 8},   // Faug: F - A - C#
  {0, 4, 8},   // F#aug: F# - A# - C##(D)
  {0, 4, 8},   // Gaug: G - B - D#
  {0, 4, 8},   // G#aug: G# - B# - D##(E)
  {0, 4, 8},   // Aaug: A - C# - E#(F)
  {0, 4, 8},   // A#aug: A# - C##(D) - E##(F#)
  {0, 4, 8},   // Baug: B - D# - F##(G)
  {0, 4, 8}    // Caug: C - E - G#
};

// ===== DIMINISHED AKKORDE =====
// Kleine Terz + Kleine Quinte - verstimmter, dunkler Sound
const int chords_dim[13][3] = {
  {0, 3, 6},   // Cdim: C - Eb - Gb
  {0, 3, 6},   // C#dim: C# - E - G
  {0, 3, 6},   // Ddim: D - F - Ab
  {0, 3, 6},   // D#dim: D# - F# - A
  {0, 3, 6},   // Edim: E - G - Bb
  {0, 3, 6},   // Fdim: F - Ab - B
  {0, 3, 6},   // F#dim: F# - A - C
  {0, 3, 6},   // Gdim: G - Bb - Db
  {0, 3, 6},   // G#dim: G# - B - D
  {0, 3, 6},   // Adim: A - C - Eb
  {0, 3, 6},   // A#dim: A# - C# - E
  {0, 3, 6},   // Bdim: B - D - F
  {0, 3, 6}    // Cdim: C - Eb - Gb
};

/* 
SEMITONE REFERENZTABELLE:
0 = Grundnote (Unison, z.B. C)
1 = Kleine Sekunde / Halbton (z.B. C#)
2 = Große Sekunde / Ganzton (z.B. D)
3 = Kleine Terz (z.B. Eb / D#)
4 = Große Terz (z.B. E)
5 = Quarte/Perfect Fourth (z.B. F)
6 = Tritonus (z.B. F#)
7 = Quinte/Perfect Fifth (z.B. G)
8 = Kleine Sexte / Übermäßige Quinte (z.B. G#)
9 = Große Sexte (z.B. A)
10 = Kleine Septime (z.B. Bb)
11 = Große Septime (z.B. B)
12 = Oktave (z.B. C nächste Oktave)

VERWENDUNG IN HALKEYBOARD.INO:
1. Kopiere eine der obigen Definitionen
2. Benenne sie in "chords" um
3. Falls du Akkorde mit 4 oder mehr Noten verwendest: ändere "maxChordNotes" von 3 auf die neue Anzahl
4. Speichere und lade den Code auf den Arduino
*/
