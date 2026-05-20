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
  chord_engine_state.last_voicing_count = 0;
  chord_engine_state.voicing_mode = 0;
  chord_engine_state.chord_palette = 0;
  chord_engine_state.latch_mode = 0;
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

  const ChordTemplate* tpl = chord_template_for_cell(chord_engine_state.chord_palette, row, col);
  if (Device.serialMode) {
    Serial.print("  palette=");Serial.print((int)chord_engine_state.chord_palette);
    Serial.print(" tpl=");Serial.println(tpl == nullptr ? "(null)" : tpl->name);
  }
  if (tpl == nullptr) return;

  uint8_t new_notes[MAX_CHORD_VOICES];
  uint8_t new_count;

  // Pick the prev voicing for parsimonious mode: held_notes wins when the user
  // is hold-and-switching, last_voicing is the fallback after a release so
  // single-press-then-release-then-press sequences (I → V → vi typed one at a
  // time) still voice-lead instead of resetting to root-position close every
  // press. Reset events (init, tonic change, palette change, preset load)
  // clear last_voicing_count so the first chord after a reset always starts
  // fresh from CHORD_BASE_OCTAVE.
  const uint8_t* prev_notes;
  uint8_t prev_count;
  if (chord_engine_state.held_note_count > 0) {
    prev_notes = chord_engine_state.held_notes;
    prev_count = chord_engine_state.held_note_count;
  } else {
    prev_notes = chord_engine_state.last_voicing;
    prev_count = chord_engine_state.last_voicing_count;
  }

  bool use_parsimonious = (chord_engine_state.voicing_mode == 1) && (prev_count > 0);

  if (use_parsimonious) {
    // Parsimonious: voice_lead from prev. The spread is *inherited* — prev
    // was placed in spread form on the initial press, and voice_lead's
    // nearest-PC assignment keeps voices near those positions. Re-applying
    // apply_spread here would compound the octave drops and drift voices
    // out of range over repeated chord changes.
    uint8_t target_pcs[MAX_CHORD_VOICES];
    uint8_t pc_count = compute_chord_pcs(tpl, chord_engine_state.current_tonic_pc, target_pcs);
    new_count = voice_lead(prev_notes, prev_count, target_pcs, pc_count, new_notes);
  } else {
    // Close-position mode OR first press after reset. Compute the fresh
    // close-position voicing and apply spread to set the baseline.
    new_count = compute_chord_notes(tpl, chord_engine_state.current_tonic_pc,
                                    CHORD_BASE_OCTAVE, new_notes);
    apply_spread(new_notes, new_count, chord_engine_state.voice_spread);
  }
  if (new_count == 0) return;

  // Same-cell re-press in latch mode → retrigger: skip the common-tone
  // optimization so every voice gets a fresh note-off + note-on, even when
  // the new voicing is identical to the held one.
  bool force_retrigger = chord_engine_state.latch_mode
                      && chord_engine_state.sounding_cell_row == (int8_t)row
                      && chord_engine_state.sounding_cell_col == (int8_t)col;

  // Incremental transition: note-off departing voices, note-on new voices,
  // hold common ones. This is the common path for both voicing modes; it
  // makes close-position smooth-when-it-can and parsimonious explicit.
  for (uint8_t i = 0; i < chord_engine_state.held_note_count; ++i) {
    boolean keep = false;
    if (!force_retrigger) {
      for (uint8_t j = 0; j < new_count; ++j) {
        if (chord_engine_state.held_notes[i] == new_notes[j]) { keep = true; break; }
      }
    }
    if (!keep) {
      midiSendNoteOff(LEFT, chord_engine_state.held_notes[i], CHORD_CHANNEL);
      midiSendNoteOffRaw(chord_engine_state.held_notes[i], 0x40, CHORD_CHANNEL - 1);
    }
  }
  byte emit_velocity = chord_engine_state.latch_mode ? CHORD_LATCH_VELOCITY : velocity;
  for (uint8_t i = 0; i < new_count; ++i) {
    boolean was_held = false;
    if (!force_retrigger) {
      for (uint8_t j = 0; j < chord_engine_state.held_note_count; ++j) {
        if (new_notes[i] == chord_engine_state.held_notes[j]) { was_held = true; break; }
      }
    }
    if (!was_held) {
      midiSendNoteOn(LEFT, new_notes[i], emit_velocity, CHORD_CHANNEL);
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

  // Stash for the next press's voice-leading reference — survives release.
  for (uint8_t i = 0; i < new_count; ++i) chord_engine_state.last_voicing[i] = new_notes[i];
  chord_engine_state.last_voicing_count = new_count;

  // Fast-pulsing white highlight on LED_LAYER_PLAYED, composited on the
  // function color in LED_LAYER_MAIN. Cleared on release.
  setLed(col + 1, row, COLOR_WHITE, cellFastPulse, LED_LAYER_PLAYED);
}

// Recompute the currently-sounding cell's voicing using the current spread
// setting, and diff against held_notes for clean note-on/note-off.
// Always starts from close position (ignores prev voice-leading state) so the
// voice positions reflect the new spread shape rather than inheriting the old
// one — used when the user cycles spread mid-chord. Caller is responsible for
// the no-sounding-cell case (where last_voicing should be cleared instead).
void chordEngineRevoiceCurrent() {
  if (chord_engine_state.sounding_cell_row == -1) return;

  uint8_t row = chord_engine_state.sounding_cell_row;
  uint8_t col = chord_engine_state.sounding_cell_col;
  const ChordTemplate* tpl = chord_template_for_cell(chord_engine_state.chord_palette, row, col);
  if (tpl == nullptr) return;

  uint8_t new_notes[MAX_CHORD_VOICES];
  uint8_t new_count = compute_chord_notes(tpl, chord_engine_state.current_tonic_pc,
                                          CHORD_BASE_OCTAVE, new_notes);
  apply_spread(new_notes, new_count, chord_engine_state.voice_spread);
  if (new_count == 0) return;

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
      midiSendNoteOn(LEFT, new_notes[i], CHORD_LATCH_VELOCITY, CHORD_CHANNEL);
    }
  }

  for (uint8_t i = 0; i < new_count; ++i) chord_engine_state.held_notes[i] = new_notes[i];
  chord_engine_state.held_note_count = new_count;
  for (uint8_t i = 0; i < new_count; ++i) chord_engine_state.last_voicing[i] = new_notes[i];
  chord_engine_state.last_voicing_count = new_count;
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
  // Latch: keep the chord sounding past the touch release. The next chord-cell
  // press will diff/transition off this state; toggling latch off releases it.
  if (chord_engine_state.latch_mode) return;
  chordEngineReleaseHeldChord();
}

// ---------- tonic strip ----------

// tonic_cell_to_pc[][] now lives in ls_chord_config.h so the LED painter in
// ls_chord_leds.ino can read it.

// Reserved-cell coordinates within the tonic strip (local_row, local_col).
// Row 1 cols 4..7 are sentinel (0xFF) in tonic_cell_to_pc. Bound actions:
//   row 1 col 4 → toggle voicing mode (close ↔ parsimonious)
//   row 1 col 5 → cycle voice spread (tight → drop bass → spread → tight)
//   row 1 col 6 → cycle chord palette (Pop → Jazz → Pop)
//   row 1 col 7 → toggle latch mode (off → hold-until-next-or-disabled → off)
#define TONIC_RESERVED_VOICING_COL  4
#define TONIC_RESERVED_SPREAD_COL   5
#define TONIC_RESERVED_PALETTE_COL  6
#define TONIC_RESERVED_LATCH_COL    7

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
      // Re-voice the currently-sounding chord with the new spread so the voice
      // positions reflect the new shape, and parsimonious voice-leading from
      // here forward starts from the new baseline instead of inheriting the
      // old spread. If nothing is sounding, just drop last_voicing so the next
      // press starts fresh from close + new spread.
      if (chord_engine_state.sounding_cell_row != -1) {
        chordEngineRevoiceCurrent();
      } else {
        chord_engine_state.last_voicing_count = 0;
      }
      tonic_strip_repaint();
    } else if (local_row == 1 && local_col == TONIC_RESERVED_PALETTE_COL) {
      // Release any held chord — switching palette mid-hold would leave the
      // sounding voices keyed off a template that no longer matches the cell.
      if (chord_engine_state.sounding_cell_row != -1) {
        chordEngineReleaseHeldChord();
      }
      chord_engine_state.chord_palette = (chord_engine_state.chord_palette + 1) % CHORD_PALETTE_COUNT;
      Global.chord_palette = chord_engine_state.chord_palette;
      // Fresh palette = fresh start; don't voice-lead from the previous palette's voicing.
      chord_engine_state.last_voicing_count = 0;
      tonic_strip_repaint();
    } else if (local_row == 1 && local_col == TONIC_RESERVED_LATCH_COL) {
      if (chord_engine_state.latch_mode) {
        // Turning latch off: release any currently-sounding (latched) chord.
        if (chord_engine_state.sounding_cell_row != -1) {
          chordEngineReleaseHeldChord();
        }
        chord_engine_state.latch_mode = 0;
      } else {
        chord_engine_state.latch_mode = 1;
      }
      Global.chord_latch_mode = chord_engine_state.latch_mode;
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
  // Fresh key = fresh start; voice-leading from a voicing in the old key
  // would project the old voices onto the new tonic's PCs and produce surprising
  // inversions.
  chord_engine_state.last_voicing_count = 0;

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
