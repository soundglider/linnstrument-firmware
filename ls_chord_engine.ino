/**************************** ls_chord_engine: runtime state + zone dispatch *********************
Single state struct, zone dispatch out of handleNewTouch/handleTouchRelease, and the three
zone touch handlers (chord grid, tonic strip, melody zone). Chord-grid press logic is the
only zone with full v1 behavior wired up; tonic strip and melody zone are still stubs that
log over serial.

See docs/ARCHITECTURE.md §Runtime state, §Zone dispatch, §Touch lifecycles.
**************************************************************************************************/

#include "ls_chord_config.h"
#include "ls_chord_vocab.h"

// Forward decls — Arduino IDE 1.8's auto-prototype generator can miss the
// macro-sized array parameter, and these live in alphabetically-later files.
uint8_t compute_chord_notes(const ChordTemplate* tpl, uint8_t tonic_pc,
                            uint8_t base_octave, uint8_t out_notes[]);
uint8_t compute_chord_pcs(const ChordTemplate* tpl, uint8_t tonic_pc,
                          uint8_t out_pcs[]);
uint8_t voice_lead(const uint8_t* prev_notes, uint8_t prev_count,
                   const uint8_t* target_pcs, uint8_t pc_count,
                   uint8_t out_notes[]);
void apply_spread(uint8_t* notes, uint8_t count, uint8_t level);
void chord_grid_repaint();
void tonic_strip_repaint();

ChordEngineState chord_engine_state;

// No auto-save: chord-engine state changes update Global.chord_* so a
// user-initiated preset save (stock's flow) captures them, but we never
// schedule flash writes ourselves. Stock's writeSettingsToFlash blanks the
// LEDs while the SAM3X flash controller stalls the CPU, which is visually
// jarring mid-play; pushing that cost into explicit user-driven saves keeps
// it predictable.

// Predicate: is this (sensor) cell controlled by the chord engine?
// Chord grid (cols 1..8) is active when Split[LEFT].chordMode is on.
// Tonic strip (cols 9..16, rows 0..1) is active when Split[RIGHT].chordTonicStrip is on.
// Col 0 (command column) and the melody zone (cols 9..16, rows 2..7) always fall
// through to stock. The per-split flags can be toggled in displayPerSplit mode.
boolean isChordEngineZone(byte col, byte row) {
  if (col == 0) return false;
  if (col <= CHORD_GRID_COLS) return Split[LEFT].chordMode;
  if (row < TONIC_STRIP_ROWS) return Split[RIGHT].chordTonicStrip;
  return false;
}

void chordEngineInit() {
  chord_engine_state.current_tonic_pc = 0;
  chord_engine_state.sounding_cell_row = -1;
  chord_engine_state.sounding_cell_col = -1;
  chord_engine_state.held_note_count = 0;
  chord_engine_state.voicing_mode = 0;
  chord_engine_state.melody_layout_mode = 0;
  for (uint8_t i = 0; i < MAX_CHORD_VOICES; ++i) {
    chord_engine_state.held_notes[i] = 0;
  }

  // Initial LED paint of our zones. The melody zone is painted by stock
  // (paintNormalDisplaySplit) per the user's per-split note-light settings.
  chord_grid_repaint();
  tonic_strip_repaint();
}

// ---------- chord grid ----------

void chordEngineReleaseHeldChord() {
  for (uint8_t i = 0; i < chord_engine_state.held_note_count; ++i) {
    byte n = chord_engine_state.held_notes[i];
    // Standard note-off (gated by stock's lastValueMidiNotesOn counter).
    midiSendNoteOff(LEFT, n, CHORD_CHANNEL);
    // Defensive raw note-off — bypasses the counter gating in midiSendNoteOff
    // so the note-off goes out even if the counter is desynced from a stock-
    // path interaction. Channel here is 0-indexed.
    midiSendNoteOffRaw(n, 0x40, CHORD_CHANNEL - 1);
  }

  // Drop the sounding-cell white highlight (LED_LAYER_PLAYED composites over
  // the function-color base in LED_LAYER_MAIN).
  if (chord_engine_state.sounding_cell_row != -1) {
    byte sensorCol = chord_engine_state.sounding_cell_col + 1;
    byte sensorRow = chord_engine_state.sounding_cell_row;
    clearLed(sensorCol, sensorRow, LED_LAYER_PLAYED);
  }

  chord_engine_state.held_note_count = 0;
  chord_engine_state.sounding_cell_row = -1;
  chord_engine_state.sounding_cell_col = -1;
}

void chordEngineHandleTouchOn(uint8_t col, uint8_t row, uint8_t velocity) {
  // Only print debug logs in serial mode — in MIDI mode the USB endpoint is a
  // MIDI Class device, and any Serial.print bytes get interpreted as MIDI data
  // (via running-status), producing stuck notes in the high octaves.
  if (Device.serialMode) {
    Serial.print("chord on  col=");Serial.print((int)col);
    Serial.print(" row=");Serial.print((int)row);
    Serial.print(" vel=");Serial.println((int)velocity);
  }

  const ChordTemplate* tpl = chord_template_for_cell(row, col);
  if (tpl == nullptr) return;

  uint8_t new_notes[MAX_CHORD_VOICES];
  uint8_t new_count;

  bool use_parsimonious = (chord_engine_state.voicing_mode == 1)
                       && (chord_engine_state.held_note_count > 0);

  if (use_parsimonious) {
    uint8_t target_pcs[MAX_CHORD_VOICES];
    uint8_t pc_count = compute_chord_pcs(tpl, chord_engine_state.current_tonic_pc, target_pcs);
    new_count = voice_lead(chord_engine_state.held_notes, chord_engine_state.held_note_count,
                           target_pcs, pc_count, new_notes);
  } else {
    new_count = compute_chord_notes(tpl, chord_engine_state.current_tonic_pc,
                                    CHORD_BASE_OCTAVE, new_notes);
  }
  if (new_count == 0) return;

  // Voice-spread post-process (octave shifts for bass / top). Applied to
  // both voicing modes so the user can spread wide regardless of leading.
  apply_spread(new_notes, new_count, chord_engine_state.voice_spread);

  // Incremental transition: note-off departing voices, note-on new voices,
  // hold common ones. This is the common path for both voicing modes; it
  // makes close-position smooth-when-it-can and parsimonious explicit.
  for (uint8_t i = 0; i < chord_engine_state.held_note_count; ++i) {
    boolean keep = false;
    for (uint8_t j = 0; j < new_count; ++j) {
      if (chord_engine_state.held_notes[i] == new_notes[j]) { keep = true; break; }
    }
    if (!keep) {
      midiSendNoteOff(LEFT, chord_engine_state.held_notes[i], CHORD_CHANNEL);
      midiSendNoteOffRaw(chord_engine_state.held_notes[i], 0x40, CHORD_CHANNEL - 1);
    }
  }
  for (uint8_t i = 0; i < new_count; ++i) {
    boolean was_held = false;
    for (uint8_t j = 0; j < chord_engine_state.held_note_count; ++j) {
      if (new_notes[i] == chord_engine_state.held_notes[j]) { was_held = true; break; }
    }
    if (!was_held) {
      midiSendNoteOn(LEFT, new_notes[i], velocity, CHORD_CHANNEL);
    }
  }

  // Move the white sounding-cell highlight to the new cell.
  if (chord_engine_state.sounding_cell_row != -1) {
    byte sCol = chord_engine_state.sounding_cell_col + 1;
    byte sRow = chord_engine_state.sounding_cell_row;
    clearLed(sCol, sRow, LED_LAYER_PLAYED);
  }

  for (uint8_t i = 0; i < new_count; ++i) chord_engine_state.held_notes[i] = new_notes[i];
  chord_engine_state.held_note_count = new_count;
  chord_engine_state.sounding_cell_row = row;
  chord_engine_state.sounding_cell_col = col;

  // Fast-pulsing white highlight on LED_LAYER_PLAYED, composited on the
  // function color in LED_LAYER_MAIN. Cleared on release.
  setLed(col + 1, row, COLOR_WHITE, cellFastPulse, LED_LAYER_PLAYED);
}

void chordEngineHandleTouchOff(uint8_t col, uint8_t row) {
  if (Device.serialMode) {
    Serial.print("chord off col=");Serial.print((int)col);
    Serial.print(" row=");Serial.println((int)row);
  }

  if (chord_engine_state.sounding_cell_row != (int8_t)row ||
      chord_engine_state.sounding_cell_col != (int8_t)col) {
    // an older cell whose release came in after a newer chord took over
    return;
  }
  chordEngineReleaseHeldChord();
}

// ---------- tonic strip ----------

// tonic_cell_to_pc[][] now lives in ls_chord_config.h so the LED painter in
// ls_chord_leds.ino can read it.

// Reserved-cell coordinates within the tonic strip (local_row, local_col).
// Row 1 cols 4..7 are sentinel (0xFF) in tonic_cell_to_pc. Bound actions:
//   row 1 col 4 → toggle voicing mode (close ↔ parsimonious)
//   row 1 col 5 → cycle voice spread (tight → drop bass → spread → tight)
//   row 1 cols 6..7 → unbound (reserved for future)
#define TONIC_RESERVED_VOICING_COL  4
#define TONIC_RESERVED_SPREAD_COL   5

void tonicStripHandleTouchOn(uint8_t local_col, uint8_t local_row) {
  if (local_row >= 2 || local_col >= 8) return;

  uint8_t new_pc = tonic_cell_to_pc[local_row][local_col];
  if (new_pc == 0xFF) {
    // Reserved cell. Dispatch to bound action.
    if (local_row == 1 && local_col == TONIC_RESERVED_VOICING_COL) {
      chord_engine_state.voicing_mode = chord_engine_state.voicing_mode ? 0 : 1;
      Global.chord_voicing_mode = chord_engine_state.voicing_mode;
      tonic_strip_repaint();
    } else if (local_row == 1 && local_col == TONIC_RESERVED_SPREAD_COL) {
      chord_engine_state.voice_spread = (chord_engine_state.voice_spread + 1) % 3;
      Global.chord_voice_spread = chord_engine_state.voice_spread;
      tonic_strip_repaint();
    }
    return;
  }

  // Release any held chord before re-keying so we don't leave the previous
  // chord ringing in the old tonic while the new tonic takes effect.
  if (chord_engine_state.sounding_cell_row != -1) {
    chordEngineReleaseHeldChord();
  }

  chord_engine_state.current_tonic_pc = new_pc;
  Global.chord_current_tonic_pc = new_pc;

  if (Device.serialMode) {
    Serial.print("tonic = pc ");Serial.println((int)new_pc);
  }

  // Move the white tonic highlight to the new cell. Chord-grid colors are
  // tonic-independent. The melody zone uses stock per-split note-light
  // settings (independent of our tonic), so we don't repaint it here.
  tonic_strip_repaint();
}

void tonicStripHandleTouchOff(uint8_t local_col, uint8_t local_row) {
  // Tonic strip is state-setting only on press; release is a no-op in v1.
  (void)local_col; (void)local_row;
}

// ---------- melody zone ----------
// Melody-zone touches and LED paint are delegated to stock LinnStrument
// behavior (per-split note-lights, channel, scale, etc.) — see
// handleNewTouch_zoneDispatch below, which returns false for melody cells
// so they fall through to the stock note-resolution path.

// ---------- zone dispatch ----------

// Stock firmware reserves col 0 for command buttons and uses cols 1..16 as the playable
// surface. docs/LAYOUT.md numbers the playable surface 0..15, so we shift by 1 here.
boolean handleNewTouch_zoneDispatch(byte col, byte row, byte velocity) {
  // Only intercept cells in normal play mode. In settings / calibration /
  // sequencer / per-split / etc. modes, fall through so the user can use the
  // surface for menu navigation as stock intends.
  if (displayMode != displayNormal) return false;

  if (col == 0) return false;
  byte ccol = col - 1;
  if (ccol < CHORD_GRID_COLS) {
    chordEngineHandleTouchOn(ccol, row, velocity);
    return true;
  }
  if (row < TONIC_STRIP_ROWS) {
    tonicStripHandleTouchOn(ccol - CHORD_GRID_COLS, row);
    return true;
  }
  // melody zone — let stock LinnStrument handle this cell normally
  return false;
}

boolean handleTouchRelease_zoneDispatch(byte col, byte row) {
  if (col == 0) return false;
  byte ccol = col - 1;
  // Always allow chord-grid release through. chordEngineHandleTouchOff is
  // self-gating on sounding_cell_*, so it's a no-op when the cell isn't the
  // currently-sounding one. This is important: if the user entered a settings
  // mode mid-chord-hold OR toggled chord mode off mid-hold, the eventual
  // release still needs to clean up our state and send note-offs.
  if (ccol < CHORD_GRID_COLS) {
    chordEngineHandleTouchOff(ccol, row);
    return true;
  }
  // Tonic strip release is a v1 no-op, so it's safe to skip when out of
  // normal mode or when the flag is off.
  if (displayMode == displayNormal && row < TONIC_STRIP_ROWS && Split[RIGHT].chordTonicStrip) {
    tonicStripHandleTouchOff(ccol - CHORD_GRID_COLS, row);
    return true;
  }
  return false;
}
