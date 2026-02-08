/**
 * HOLD MODE LAYER
 * 
 * Isolierte Logik für Hold Mode:
 * - Normal Mode: Eine Note halten (toggle)
 * - Additive Mode: Mehrere Noten gleichzeitig
 * 
 * INPUT:
 *   - switch_triggered[], switch_released[] vom Hardware Controller
 *   - playModeType aus Software Controller
 *   - MIDI Note (von Hardware Controller)
 * 
 * OUTPUT:
 *   - holdModeMidiNotes[] - Welche Noten sollen vom Hold Mode gespielt werden
 */

#ifndef HOLD_MODE_H
#define HOLD_MODE_H

// ============================================
// HOLD MODE STATE
// ============================================

#define HOLD_MODE_NORMAL 0
#define HOLD_MODE_ADDITIVE 1

// Tracke welche Noten im Hold Mode gehalten werden
uint8_t holdModeMidiNotes[16];       
int8_t lastHeldNote = -1;               // Letzte gehaltene Note (für Normal Mode)

#define IS_HOLD_NOTE_ACTIVE(n) ((holdModeMidiNotes[(n) >> 3] >> ((n) & 7)) & 1)
#define SET_HOLD_NOTE_ACTIVE(n, v) if(v) holdModeMidiNotes[(n) >> 3] |= (1 << ((n) & 7)); else holdModeMidiNotes[(n) >> 3] &= ~(1 << ((n) & 7))

// ============================================
// HOLD MODE FUNCTIONS
// ============================================

void initHoldMode() {
  for (int i = 0; i < 16; i++) {
    holdModeMidiNotes[i] = 0;
  }
  lastHeldNote = -1;
}

/**
 * Verarbeite Taste-Event im Hold Mode
 * 
 * Input: 
 *   - switchIndex: Welcher Switch wurde gedrückt (0-11)
 *   - midiNote: MIDI Note für diesen Switch
 *   - isPressed: true=Taste gerade gedrückt, false=losgelassen
 * 
 * Output:
 *   - Aktualisiert holdModeMidiNotes[] Array
 */
void updateHoldMode(int switchIndex, int midiNote, bool isPressed) {
  // Bestimme den aktiven Hold Mode aus dem Hauptprogramm
  extern bool additiveMode;
  extern bool holdMode;
  
  // Wenn Hold Mode nicht aktiv, nicht verarbeiten
  if (!holdMode) {
    // Nur direkt spielen bei nicht-Hold
    return;
  }
  
  if (additiveMode) {
    // ========================================
    // ADDITIVE MODE: Mehrere Noten gleichzeitig
    // ========================================
    if (isPressed) {
      // Toggle-Check: Wenn Note schon an, ausschalten
      if (IS_HOLD_NOTE_ACTIVE(midiNote)) {
        SET_HOLD_NOTE_ACTIVE(midiNote, false);
        //Serial.print("Hold Note Off (Additive Toggle): ");
      } else {
        // Note anschalten
        SET_HOLD_NOTE_ACTIVE(midiNote, true);
        //Serial.print("Hold Note On (Additive): ");
      }
    }
  } else {
    // ========================================
    // NORMAL MODE: Eine Note nach der anderen (toggle)
    // ========================================
    if (isPressed) {
      if (lastHeldNote == midiNote) {
        // Gleiche Note nochmal - ausschalten (toggle)
        SET_HOLD_NOTE_ACTIVE(midiNote, false);
        lastHeldNote = -1;
        //Serial.print("Hold Note Off (Toggle): ");
      } else {
        // Neue Note - schalte alte aus, neue an
        if (lastHeldNote >= 0 && lastHeldNote < 128) {
          SET_HOLD_NOTE_ACTIVE(lastHeldNote, false);
          //Serial.print("Hold Note Off (Old): ");
        }
        
        SET_HOLD_NOTE_ACTIVE(midiNote, true);
        lastHeldNote = midiNote;
      }
    }
    // Wenn Released: Nichts tun! Note wird durch nächsten Druck toggled
    // Das ist das "Hold" Verhalten
  }
}

/**
 * Gib Array von aktiven Hold Mode Noten
 * Diese werden an den MIDI Generator übergeben
 */
uint8_t* getHoldModeNotes() {
  return holdModeMidiNotes;
}

/**
 * Resets all held notes (z.B. wenn Hold Mode deaktiviert wird)
 */
void clearHoldMode() {
  for (int i = 0; i < 16; i++) {
    holdModeMidiNotes[i] = 0;
  }
  lastHeldNote = -1;
}

#endif
