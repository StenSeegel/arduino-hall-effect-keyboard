# MIDI Clock INPUT Implementation

## Übersicht

MIDI Clock INPUT wurde erfolgreich in HallKeyboard.ino integriert. Das System unterstützt jetzt bidirektionalen MIDI Clock Betrieb:
- **INPUT:** Empfängt externe MIDI Clock (24 PPQN)
- **OUTPUT:** Sendet weiterhin MIDI Clock (basierend auf externer oder interner Clock)

## Implementierte Komponenten

### 1. MidiClockReceiver.h (NEU)
**Zweck:** MIDI Clock Input Layer

**Funktionen:**
- `handleMidiClock()` - Interrupt Handler für MIDI Clock (0xF8)
- `handleMidiStart()` - Start Message Handler (0xFA)
- `handleMidiStop()` - Stop Message Handler (0xFC)
- `handleMidiContinue()` - Continue Handler (0xFB)
- `handleActiveSensing()` - Heartbeat Detection (0xFE)
- `updateMidiClockReceiver()` - Timeout Detection (300ms)
- `initMidiClockReceiver()` - Initialisierung

**Variablen:**
- `midiClockActive` (bool) - MIDI Clock Status
- `calculatedBPM` (float) - Berechnetes BPM aus MIDI Clock
- `receivedPpqnCounter` (uint16_t) - PPQN Counter (0-23)

### 2. HallKeyboard.ino (MODIFIZIERT)
**Änderungen:**

#### Setup:
```cpp
MIDI.begin(MIDI_CHANNEL_OMNI);
MIDI.setHandleClock(handleMidiClock);
MIDI.setHandleStart(handleMidiStart);
MIDI.setHandleStop(handleMidiStop);
MIDI.setHandleContinue(handleMidiContinue);
MIDI.setHandleActiveSensing(handleActiveSensing);
initMidiClockReceiver();
```

#### Loop:
```cpp
MIDI.read(); // Poll MIDI Input
updateMidiClockReceiver(); // Timeout Check
```

#### Tap Tempo Priority:
```cpp
// TapTempo nur aktiv wenn kein MIDI Clock
if (!inSubmenu && !midiClockActive) {
  tapTempo.update(functionSwitches[3].isDown());
}
```

### 3. MidiClockGenerator.h (MODIFIZIERT)
**Änderung:** Intelligente BPM Quellen-Auswahl

```cpp
void updateClockInterval() {
  float bpm = midiClockActive ? calculatedBPM : tapTempo.getBPM();
  clockIntervalMicros = (60000000.0 / bpm) / 24;
}
```

### 4. LEDAnimator.h (MODIFIZIERT)
**Visuelles Feedback:**
- **MIDI Clock aktiv:** LED 7 blinkt **CYAN** im Takt
- **TapTempo aktiv:** LED 7 blinkt **WEISS** im Takt

## Verhalten

### Auto-Switching
1. **MIDI Clock erkannt:**
   - `midiClockActive = true`
   - BPM wird von MIDI Clock übernommen
   - Tap Tempo Button (FS4) deaktiviert
   - LED 7 blinkt Cyan

2. **MIDI Clock Timeout (300ms):**
   - `midiClockActive = false`
   - Fallback zu TapTempo
   - Tap Tempo Button wieder aktiv
   - LED 7 blinkt Weiß

3. **MIDI Clock OUTPUT:**
   - Läuft weiterhin basierend auf aktiver Quelle
   - Synchronisiert Arpeggiator und andere Module

## Hardware Requirements

- **MIDI IN:** Serial1 RX Pin (31250 Baud)
- **MIDI OUT:** Serial1 TX Pin (31250 Baud)
- **Library:** FortySevenEffects MIDI Library

## Testing

### Testszenarien:
1. ✅ MIDI Clock Kabel einstecken → Auto-Switch zu MIDI Clock
2. ✅ MIDI Clock Kabel entfernen → Timeout → Fallback zu TapTempo
3. ✅ BPM Änderungen via MIDI → Arpeggiator bleibt synced
4. ✅ Tap Tempo Button deaktiviert während MIDI Clock aktiv
5. ✅ LED Feedback zeigt aktive Clock-Quelle

### Debug Output:
Bereits implementiert in `updateMidiClockReceiver()`:
```cpp
Serial.println("MIDI Clock: TIMEOUT - Fallback to TapTempo");
```

Optional: Aktiviere Debug-Kommentare in MidiClockGenerator.h:
```cpp
Serial.print("MIDI Clock BPM: ");
Serial.println(bpm);
```

## Nächste Schritte (Optional)

### Phase 7: Erweiterte Features
- [ ] MIDI Clock Jitter-Filterung (gleitender Durchschnitt)
- [ ] LED Submenu für MIDI Clock Source Auswahl (Force TapTempo)
- [ ] MIDI Thru Mode (Clock durchschleifen ohne Re-Timing)
- [ ] BPM Display auf Serial Monitor

### Phase 8: Testing & Refinement
- [ ] Echtzeit-Tests mit verschiedenen MIDI Clock Quellen
- [ ] Edge Case: Sehr schnelle BPM Wechsel
- [ ] Edge Case: Jittery Clock Signale
