# Firmware Architecture

How the chord-grid firmware is structured on top of the stock LinnStrument
codebase. For the physical layout it expresses, see [LAYOUT.md](LAYOUT.md);
for the chord data, see [VOCABULARY.md](VOCABULARY.md); for the build
process, see [BUILD.md](BUILD.md).

## Approach: a real GitHub fork

The repo is a real GitHub fork of
[`rogerlinndesign/linnstrument-firmware`](https://github.com/rogerlinndesign/linnstrument-firmware).
Stock firmware sources are kept verbatim at the root of the repo, with
all new behavior confined to six new `ls_chord_*.{ino,h}` files plus a
few small targeted patches on existing stock files.

**Greenfield** in this project means greenfield *application code* on top
of an unmodified hardware-driver baseline, not a rewrite of the touch /
LED / MIDI / EEPROM stack.

### What we keep from the stock firmware

| Subsystem        | Stock file                                         |
|------------------|----------------------------------------------------|
| Touch detection  | `ls_handleTouches.ino`                             |
| LED driver       | `ls_leds.ino` (`setLed(...)`, layer compositing)   |
| MIDI send        | `ls_midi.ino` (`midiSendNoteOn/Off`)               |
| EEPROM framework | `ls_settings.ino` (preset save/load)               |
| Boot / setup     | `linnstrument-firmware.ino`                        |
| Per-split paint  | `ls_displayModes.ino` (`paintNormalDisplay`, `paintPerSplitDisplay`) |
| Calibration, switches, sensors, sequencer base classes | (unchanged) |

### What we add (six new files)

| File                      | Role                                                |
|---------------------------|-----------------------------------------------------|
| `ls_chord_config.h`       | Constants, `ChordEngineState` struct, tonic-strip PC map |
| `ls_chord_vocab.h`        | 64-entry vocabulary table + `chord_template_for_cell` — see [VOCABULARY.md](VOCABULARY.md) |
| `ls_chord_engine.ino`     | Runtime state, `isChordEngineZone`, zone dispatch, chord + tonic touch handlers |
| `ls_chord_voicing.ino`    | `compute_chord_notes`, `compute_chord_pcs`, `voice_lead`, `apply_spread` |
| `ls_chord_leds.ino`       | Painters for the chord grid and tonic strip         |
| `ls_chord_melody.ino`     | Empty stub — the melody zone is delegated to stock  |

### What we modify (small targeted patches)

| File                        | Patch                                                       |
|-----------------------------|-------------------------------------------------------------|
| `linnstrument-firmware.ino` | `setup()` calls `chordEngineInit()` after `reset()`. `GlobalSettings` and `SplitSettings` gain a few chord-engine fields. |
| `ls_handleTouches.ino`      | Zone dispatch at the top of `handleNewTouch` and `handleTouchRelease`; gates inside `handleXYZupdate` skip stock `prepareNewNote` / `sendNewNote` for chord-engine zones in normal play mode. |
| `ls_displayModes.ino`       | `paintNormalDisplay` calls our repaints after stock paint (overpainting chord-engine zones); `paintPerSplitDisplay` shows the per-split chord-mode and tonic-strip toggle indicators. |
| `ls_settings.ino`           | Defaults for new fields, preset sync in `applyPresetSettings`, press handling for the per-split toggle cells in `handlePerSplitSettingNewTouch`. |

## Runtime state

The chord engine's runtime state lives in `ChordEngineState`, declared
in `ls_chord_config.h` so the LED painter can read it:

```c++
struct ChordEngineState {
    uint8_t  current_tonic_pc;       // 0..11
    int8_t   sounding_cell_row;      // -1 = none
    int8_t   sounding_cell_col;      // -1 = none
    uint8_t  held_notes[MAX_CHORD_VOICES];
    uint8_t  held_note_count;        // 0..7
    uint8_t  voicing_mode;           // 0 = close, 1 = parsimonious
    uint8_t  voice_spread;           // 0 = tight, 1 = drop-2+4, 2 = wider
    uint8_t  melody_layout_mode;     // unused — melody is stock-delegated
};

ChordEngineState chord_engine_state;  // defined in ls_chord_engine.ino
```

Mutated by zone handlers only. Arduino is single-threaded; ISRs touch
the sensor matrix, not engine state.

The **per-split toggles** that decide whether a split's cells use chord-
engine behavior live in `SplitSettings` (in `linnstrument-firmware.ino`):

```c++
boolean chordMode;         // chord grid for cols 1..8 when on
boolean chordTonicStrip;   // tonic strip for cols 9..16 rows 0..1 when on
```

The **persisted chord-engine parameters** (tonic PC, voicing mode, voice
spread) live in `GlobalSettings` alongside other global config. They are
synced into `chord_engine_state` in `applyPresetSettings` so loading a
preset reapplies them. The chord engine does **not** auto-save; the user
explicitly committing a preset (via stock's preset UI) is what writes
them to flash.

## Zone dispatch

```c++
boolean handleNewTouch_zoneDispatch(byte col, byte row, byte velocity) {
    if (displayMode != displayNormal) return false;   // stay out of menus
    if (col == 0) return false;                       // command column
    byte ccol = col - 1;
    if (ccol < CHORD_GRID_COLS) {
        chordEngineHandleTouchOn(ccol, row, velocity);
        return true;
    }
    if (row < TONIC_STRIP_ROWS) {
        tonicStripHandleTouchOn(ccol - CHORD_GRID_COLS, row);
        return true;
    }
    return false;                                     // melody zone → stock
}
```

Stock firmware reserves `sensorCol == 0` for the command column, so the
playable surface uses cols 1..16. The dispatcher shifts by 1 internally.

`isChordEngineZone(col, row)` is the single predicate everything else
gates on:

```c++
boolean isChordEngineZone(byte col, byte row) {
    if (col == 0) return false;
    if (col <= CHORD_GRID_COLS) return Split[LEFT].chordMode;
    if (row < TONIC_STRIP_ROWS) return Split[RIGHT].chordTonicStrip;
    return false;
}
```

The fixed-position design: the chord grid always occupies cols 1..8 when
`Split[LEFT].chordMode` is on; the tonic strip always occupies cols
9..16, rows 0..1 when `Split[RIGHT].chordTonicStrip` is on. The toggles
are *per-split* (configurable independently per split) but the zone
geometry is fixed — see [LAYOUT.md](LAYOUT.md) for the rationale.

Constants come from `ls_chord_config.h`:

```c++
#define CHORD_GRID_COLS        8
#define TONIC_STRIP_ROWS       2
#define CHORD_CHANNEL          3    // chord-grid MIDI channel
#define MELODY_CHANNEL         2    // unused (melody zone is stock)
#define CHORD_BASE_OCTAVE      4    // chord notes start at MIDI 48 = C3
#define MELODY_BASE_OCTAVE     5    // unused
#define MAX_CHORD_VOICES       7
```

The release dispatcher mirrors the press dispatcher, but with one
deliberate exception: chord-grid release **always** runs the chord-cell
off-handler regardless of `displayMode` or `chordMode`. The off-handler
self-gates on `sounding_cell_*` (it's a no-op if the cell isn't the
currently-sounding one), and this property is essential — if the user
enters a settings mode mid-chord-hold, or toggles chord mode off while a
chord is held, the eventual release still has to clean up our state and
send the note-offs.

## Pure-function boundaries

The voicing layer is intentionally a set of pure functions that live in
`ls_chord_voicing.ino`. Arduino has minimal unit-test infrastructure, so
this is the seam where the engine could be tested off-target if needed.

| Function                                                                | Returns                       |
|-------------------------------------------------------------------------|-------------------------------|
| `chord_template_for_cell(row, col)`                                     | `const ChordTemplate*` (nullptr for empty cells) |
| `compute_chord_notes(template, tonic_pc, base_octave, out_notes[])`     | note count (close position)   |
| `compute_chord_pcs(template, tonic_pc, out_pcs[])`                      | pitch-class count             |
| `voice_lead(prev_notes[], prev_count, target_pcs[], pc_count, out[])`   | note count (parsimonious)     |
| `apply_spread(notes[], count, level)`                                   | in-place octave shifts        |
| `function_color(function_id)`                                           | palette index                 |
| `isChordEngineZone(col, row)`                                           | boolean                       |

State mutation happens only inside the zone handlers (`chordEngineHandleTouchOn/Off`
and `tonicStripHandleTouchOn`); everything else is read-only.

## Touch lifecycles

### Chord cell (sensorCol 1..8)

**Press — `chordEngineHandleTouchOn(local_col, row, vel)`:**

1. Resolve `tpl = chord_template_for_cell(row, local_col)`. If null (empty
   cell, e.g. vii° col 7), return.
2. Decide voicing strategy:
   - If `voicing_mode == 1` *and* `held_note_count > 0` → parsimonious:
     `voice_lead(held_notes, target_pcs, ...)` voice-leads from the
     currently-held chord to the new chord's pitch classes. The spread
     is *inherited* from prev — no re-application.
   - Otherwise → close position: `compute_chord_notes(tpl, ...)` then
     `apply_spread(...)` bakes in the spread shape on this fresh press.
3. **Incremental transition**: diff old `held_notes[]` against new notes.
   - Notes only in old → send `note_off`.
   - Notes only in new → send `note_on`.
   - Notes in both (common tones, same MIDI value) → held without retrigger.
4. Move the white pulse highlight (`LED_LAYER_PLAYED`) to the new cell.
5. Update state: `held_notes[]`, `held_note_count`, `sounding_cell_*`.

**Release — `chordEngineHandleTouchOff(local_col, row)`:**

1. If `(local_col, row) != sounding_cell_*`, ignore (an older cell whose
   release came in after a newer chord took over).
2. Send `note_off` for each entry in `held_notes[]` (both via the standard
   counter-gated `midiSendNoteOff` and the raw `midiSendNoteOffRaw` as a
   defensive double-send).
3. Clear the cell's `LED_LAYER_PLAYED` highlight; reset state.

v1 is **monophonic-chord** — newest chord-cell press wins. Multi-touch
chord layering is deferred to Phase 11.

### Tonic cell (sensorCol 9..16, sensorRow 0..1)

**Press — `tonicStripHandleTouchOn(local_col, local_row)`:**

1. `new_pc = tonic_cell_to_pc[local_row][local_col]`. If the sentinel
   `0xFF` (reserved cell), dispatch to the bound action and return.
2. If a chord is currently held, release it before re-keying (so the old
   chord doesn't ring at the wrong key during the transition).
3. Update `chord_engine_state.current_tonic_pc` *and*
   `Global.chord_current_tonic_pc` (the latter lets a subsequent stock
   preset save persist the tonic).
4. Repaint the tonic strip (move the white highlight).

**Reserved cell bindings (row 1):**

- `local_col == 4` (sensorCol 13) → toggle `voicing_mode` (close ↔ parsimonious).
- `local_col == 5` (sensorCol 14) → cycle `voice_spread` (0 → 1 → 2 → 0).
- `local_col == 6, 7` → unbound (reserved for future).

The tonic strip's release handler is a v1 no-op.

### Melody zone (sensorCol 9..16, sensorRow 2..7)

The melody zone is **delegated to stock LinnStrument**. The zone
dispatcher returns `false` for cells in this zone, so they fall through
to stock note resolution: `prepareNewNote` → `sendNewNote` runs as
normal, using the right split's per-split settings (channel, row offset,
note-lights, transpose, etc.). The melody zone's LED paint also comes
from stock (`paintNormalDisplaySplit`).

This means each user can configure the melody zone however they like —
scale-aware piano-roll, guitar tuning, isomorphic, chromatic, fader
strip, anything stock supports. The chord-grid engine and the melody
zone coexist on the same surface without either constraining the other.

## Per-split toggle UI

In `displayPerSplit` mode, four indicator cells configure the chord-grid
behavior of the currently-edited split (`Global.currentPerSplit`):

| Cell (sensor coords) | Setting                      | Indicator              |
|----------------------|------------------------------|------------------------|
| col 1 row 1, col 9 row 1 | Toggle `chordMode`        | `Split[side].colorMain` when on |
| col 1 row 2, col 9 row 2 | Toggle `chordTonicStrip`  | `Split[side].colorMain` when on |

The cells appear at *both* col 1 and col 9 (an 8-col "repeat") so the
toggle is reachable from either hand without crossing the surface.
Toggling `chordMode` off mid-chord also calls `chordEngineReleaseHeldChord`
to avoid leaking notes.

## MIDI channels

| Channel | Use                                                    |
|---------|--------------------------------------------------------|
| 3       | Chord notes (chord-grid presses); `CHORD_CHANNEL`      |
| (per-split) | Melody / lead — whatever the right split's MIDI mode + channel say |
| 1..16   | Other splits / faders — stock behavior                 |

Standard `Note On` / `Note Off` from the chord grid. No pitch-bend, no
MPE on the chord channel. The melody zone uses stock's expression
machinery, so MPE / pitch bend / Y / Z all work there if configured in
per-split settings.

## LED painters

Two repaint functions in `ls_chord_leds.ino`:

| Function                  | Repaints                                                 |
|---------------------------|----------------------------------------------------------|
| `chord_grid_repaint()`    | 64 cells with row function color (no-op when `Split[LEFT].chordMode` is off — stock paint stays) |
| `tonic_strip_repaint()`   | 16 cells: 12 PC cells green, active tonic white, reserved cells dark or bound-color (no-op when `Split[RIGHT].chordTonicStrip` is off) |

The melody zone is painted by stock (`paintNormalDisplaySplit`).

Repaint is triggered by:

- `chordEngineInit()` at boot.
- `paintNormalDisplay` (stock's normal-mode entry) — first calls stock's
  per-cell paint across the whole surface, then our two repaints overpaint
  the chord-engine zones. Stock-only zones (melody, col 0) keep their
  stock paint.
- Tonic cell press (tonic strip repainted to move the highlight).
- Per-split toggle changes (full updateDisplay via the press handler).

The currently-sounding chord cell composites a fast-pulsing white
`LED_LAYER_PLAYED` over the function color in `LED_LAYER_MAIN`. The
stock LED driver composites layers via `getCombinedLedData`.

## Settings persistence

The chord-engine fields live in stock-managed settings structures:

- `SplitSettings`: `chordMode`, `chordTonicStrip` (per split).
- `GlobalSettings`: `chord_current_tonic_pc`, `chord_voicing_mode`,
  `chord_voice_spread`.

They get loaded at boot via stock's `loadSettings` → `applyConfiguration`
→ `applyPresetSettings`, where the chord-engine sync copies the loaded
values into `chord_engine_state` and triggers a tonic-strip repaint.

There is **no auto-save**. The chord engine never calls `storeSettings`
or `writeSettingsToFlash` on its own. The user committing the current
config via stock's preset UI is what writes the chord-engine state to
flash; without that, the engine resets to defaults on each boot. The
trade-off is deliberate: stock's flash-write path explicitly blanks the
LEDs while the SAM3X flash controller stalls the CPU, which is visually
jarring during play. Pushing the cost into explicit, user-driven saves
keeps it predictable.

## Configuration constants

All compile-time tunables live in `ls_chord_config.h`:

```c++
#define CHORD_GRID_COLS           8
#define TONIC_STRIP_ROWS          2
#define CHORD_CHANNEL             3
#define MELODY_CHANNEL            2   // unused; melody zone is stock-delegated
#define CHORD_BASE_OCTAVE         4
#define MELODY_BASE_OCTAVE        5
#define MAX_CHORD_VOICES          7
```

The struct definition for `ChordEngineState` and the static
`tonic_cell_to_pc[2][8]` lookup table also live in this header so the
LED painter and other consumers can read them without cross-file
externs.

## What this architecture deliberately doesn't do

- No external app communication (no MIDI-in for chord/tonic control in v1).
- No SysEx for vocabulary updates — vocabulary is compiled in.
- No multi-touch chord layering (v1 is monophonic-chord).
- No engine-side melody handler — the melody zone is delegated to stock.
- No automatic EEPROM writes — persistence is explicit, via stock's preset save.

These are scope choices, not technical limitations; see
[ROADMAP.md § Phase 11](ROADMAP.md#phase-11-deferred--polish--edge-cases).
