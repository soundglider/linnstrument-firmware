/**************************** ls_chord_leds: LED painters for all three zones ********************
Function-color row coloring for the chord grid, dim-green-plus-white-tonic for the tonic
strip. Sounding-cell highlight is handled in ls_chord_engine.ino via LED_LAYER_PLAYED.

The stock LED driver only takes a 12-entry palette (COLOR_OFF .. COLOR_PINK) and ~6 PWM
brightness levels, not raw RGB. docs/LAYOUT.md's RGB function-color values are mapped
here to palette indices; the "8 discrete brightness levels per row" tension spec from
docs/LAYOUT.md is not achievable with this driver and is omitted for v1.

Stock paintNormalDisplay is patched to call these painters for our zones instead of
the default note-light pattern.

See docs/ARCHITECTURE.md §LED painters and docs/LAYOUT.md §LED scheme.
**************************************************************************************************/

#include "ls_chord_config.h"
#include "ls_chord_vocab.h"

// Primary family (red/orange) for the pop-progression rows I, IV, V;
// secondary family (white/cyan) for the rest. Within each row, sensor cols 1
// and 3 (local 0 and 2) take the highlight color.
static byte chord_cell_color(uint8_t row, uint8_t local_col) {
  bool is_primary   = (row == 7 || row == 4 || row == 3);
  bool is_highlight = (local_col == 0 || local_col == 2);
  if (is_primary)  return is_highlight ? COLOR_RED   : COLOR_ORANGE;
  else             return is_highlight ? COLOR_WHITE : COLOR_CYAN;
}

// Paint the 64 chord-grid cells (sensorCol 1..8, sensorRow 0..7).
// Empty cells (vocabulary entries with interval_count == 0) go dark.
// Skipped entirely when Split[LEFT].chordMode is off — stock paint stays.
void chord_grid_repaint() {
  if (!Split[LEFT].chordMode) return;
  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t local_col = 0; local_col < CHORD_GRID_COLS; ++local_col) {
      const ChordTemplate* tpl = chord_template_for_cell(chord_engine_state.chord_palette, row, local_col);
      byte sensor_col = local_col + 1;
      if (tpl == nullptr) {
        setLed(sensor_col, row, COLOR_OFF, cellOff, LED_LAYER_MAIN);
      } else {
        setLed(sensor_col, row, chord_cell_color(row, local_col), cellOn, LED_LAYER_MAIN);
      }
    }
  }
}

// Paint the 16 tonic-strip cells (sensorCol 9..16, sensorRow 0..1):
// 12 PC cells dim green, the active tonic bright white, reserved cells dark
// (except the voicing-toggle cell — orange when parsimonious is on).
// Skipped entirely when Split[RIGHT].chordTonicStrip is off — stock paint stays.
void tonic_strip_repaint() {
  if (!Split[RIGHT].chordTonicStrip) return;
  for (uint8_t local_row = 0; local_row < TONIC_STRIP_ROWS; ++local_row) {
    for (uint8_t local_col = 0; local_col < CHORD_GRID_COLS; ++local_col) {
      uint8_t pc = tonic_cell_to_pc[local_row][local_col];
      byte sensor_col = local_col + 1 + CHORD_GRID_COLS;
      if (pc == 0xFF) {
        // Reserved cells with specific bindings light up; others stay dark.
        //   row 1 col 4 → voicing mode (orange when parsimonious)
        //   row 1 col 5 → voice spread (lime at level 1, yellow at level 2)
        //   row 1 col 6 → chord palette (blue = Pop, pink = Jazz)
        //   row 1 col 7 → latch mode (red when on)
        byte rcolor = COLOR_OFF;
        CellDisplay rdisp = cellOff;
        if (local_row == 1 && local_col == 4 && chord_engine_state.voicing_mode == 1) {
          rcolor = COLOR_ORANGE; rdisp = cellOn;
        } else if (local_row == 1 && local_col == 5) {
          if (chord_engine_state.voice_spread == 1) {
            rcolor = COLOR_LIME; rdisp = cellOn;
          } else if (chord_engine_state.voice_spread == 2) {
            rcolor = COLOR_YELLOW; rdisp = cellOn;
          }
        } else if (local_row == 1 && local_col == 6) {
          rcolor = (chord_engine_state.chord_palette == CHORD_PALETTE_JAZZ) ? COLOR_PINK : COLOR_BLUE;
          rdisp = cellOn;
        } else if (local_row == 1 && local_col == 7 && chord_engine_state.latch_mode) {
          rcolor = COLOR_RED; rdisp = cellOn;
        }
        setLed(sensor_col, local_row, rcolor, rdisp, LED_LAYER_MAIN);
      } else if (pc == chord_engine_state.current_tonic_pc) {
        setLed(sensor_col, local_row, COLOR_WHITE, cellOn, LED_LAYER_MAIN);
      } else {
        setLed(sensor_col, local_row, COLOR_GREEN, cellOn, LED_LAYER_MAIN);
      }
    }
  }
}

// Melody-zone painting is delegated to stock LinnStrument's
// paintNormalDisplaySplit so the user's per-split note-light settings apply.
