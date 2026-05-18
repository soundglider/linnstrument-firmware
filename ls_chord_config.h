/**************************** ls_chord_config: chord-grid firmware constants *********************
Build-time tunables for the chord-grid fork. Everything that affects zone geometry,
MIDI routing, base octaves, or EEPROM behavior is set here so the rest of the
chord engine reads symbolically.

See docs/ARCHITECTURE.md §Configuration constants.
**************************************************************************************************/

#ifndef LS_CHORD_CONFIG_H_
#define LS_CHORD_CONFIG_H_

#define CHORD_GRID_COLS           8
#define TONIC_STRIP_ROWS          2

#define CHORD_CHANNEL             3
#define MELODY_CHANNEL            2   // unused — melody zone follows stock per-split settings

#define CHORD_BASE_OCTAVE         4
#define MELODY_BASE_OCTAVE        5

#define MAX_CHORD_VOICES          7

#define EEPROM_TONIC_DEBOUNCE_MS  3000

#include <stdint.h>

// Tonic strip cell → pitch class mapping. Sentinel 0xFF = reserved cell
// (row 1, cols 4..7 — earmarked for later phase bindings).
constexpr uint8_t tonic_cell_to_pc[2][8] = {
  { 0, 1,  2,  3,    4,    5,    6,    7    },
  { 8, 9, 10, 11, 0xFF, 0xFF, 0xFF, 0xFF }
};

// Runtime state for the chord engine. Defined in ls_chord_engine.ino.
struct ChordEngineState {
  uint8_t  current_tonic_pc;
  int8_t   sounding_cell_row;
  int8_t   sounding_cell_col;
  uint8_t  held_notes[MAX_CHORD_VOICES];
  uint8_t  held_note_count;
  uint8_t  voicing_mode;
  uint8_t  voice_spread;       // 0 = tight, 1 = drop bass, 2 = spread bass + top
  uint8_t  melody_layout_mode;
};

#endif
