/**************************** ls_chord_vocab: 8x8 chord vocabulary tables *************************
Two hand-authored 64-cell chord palettes (Pop and Jazz). Selected at runtime via
chord_engine_state.chord_palette (mirrored from Global.chord_palette for preset
persistence). Row index uses the physical row convention (row 0 = bottom = bVII,
row 7 = top = I), so the C++ literals appear in reverse vs the top-down tables
in docs/VOCABULARY.md.

Column scheme (universal across palettes — see docs/VOCABULARY.md §Column scheme):
  col 0 = triad, col 1 = sus, col 2 = 7th chord, col 7 = altered/wildcard.
  Cols 3-6 are function-aware (diatonic T/PD ascend add9 / 6 / 9 / 6-9-or-11;
  dominant V uses 9 / 13 / b9 / #9; diminished vii° has no natural sus and
  keeps its col 7 empty).
**************************************************************************************************/

#ifndef LS_CHORD_VOCAB_H_
#define LS_CHORD_VOCAB_H_

#define FN_T   0
#define FN_PD  1
#define FN_D   2
#define FN_X   3

#define ST_POP        0
#define ST_CHOIR      1
#define ST_JAZZ       2
#define ST_ELECTRONIC 3
#define ST_MODAL      4

enum ChordPaletteId : uint8_t {
  CHORD_PALETTE_POP   = 0,
  CHORD_PALETTE_JAZZ  = 1,
  CHORD_PALETTE_COUNT = 2,
};

struct ChordTemplate {
  uint8_t      root_offset;
  uint8_t      interval_count;
  uint8_t      intervals[7];
  uint8_t      function;
  uint8_t      style;
  const char*  name;
};

#define EMPTY_TEMPLATE { 0, 0, {0,0,0,0,0,0,0}, FN_T, ST_POP, "" }

constexpr ChordTemplate vocabularies[CHORD_PALETTE_COUNT][8][8] = {
  // ============================================================================
  // CHORD_PALETTE_POP — reordered current vocabulary into the universal column
  // scheme: col 0=triad, col 1=sus, col 2=7, then function-aware extensions.
  // ============================================================================
  {
    // row 0 = bVII (modal mixture, function X, root_offset = 10)
    {
      { 10, 3, {0,4,7,0,0,0,0},      FN_X, ST_MODAL,      "bVII"      },
      { 10, 3, {0,2,7,0,0,0,0},      FN_X, ST_MODAL,      "bVIIsus2"  },
      { 10, 4, {0,4,7,10,0,0,0},     FN_X, ST_MODAL,      "bVII7"     },
      { 10, 4, {0,4,7,14,0,0,0},     FN_X, ST_MODAL,      "bVIIadd9"  },
      { 10, 4, {0,4,7,9,0,0,0},      FN_X, ST_MODAL,      "bVII6"     },
      { 10, 5, {0,4,7,10,14,0,0},    FN_X, ST_JAZZ,       "bVII9"     },
      { 10, 4, {0,5,7,10,0,0,0},     FN_X, ST_MODAL,      "bVII7sus4" },
      { 10, 4, {0,4,7,11,0,0,0},     FN_X, ST_MODAL,      "bVIImaj7"  },
    },
    // row 1 = vii deg (leading-tone, function D, root_offset = 11). col 7 is empty.
    {
      { 11, 3, {0,3,6,0,0,0,0},      FN_D, ST_JAZZ,       "vii°"      },
      { 11, 4, {0,1,3,6,0,0,0},      FN_D, ST_MODAL,      "vii°(b9)"  },
      { 11, 4, {0,3,6,10,0,0,0},     FN_D, ST_JAZZ,       "viiø7"     },
      { 11, 5, {0,3,6,10,14,0,0},    FN_D, ST_JAZZ,       "viiø9"     },
      { 11, 6, {0,3,6,10,14,17,0},   FN_D, ST_JAZZ,       "viiø11"    },
      { 11, 4, {0,3,6,9,0,0,0},      FN_D, ST_JAZZ,       "viio7"     },
      { 11, 5, {0,1,3,6,9,0,0},      FN_D, ST_MODAL,      "viio7(b9)" },
      EMPTY_TEMPLATE,
    },
    // row 2 = vi (submediant, function T, root_offset = 9)
    {
      {  9, 3, {0,3,7,0,0,0,0},      FN_T, ST_POP,        "vi"        },
      {  9, 3, {0,2,7,0,0,0,0},      FN_T, ST_CHOIR,      "visus2"    },
      {  9, 4, {0,3,7,10,0,0,0},     FN_T, ST_JAZZ,       "vim7"      },
      {  9, 4, {0,3,7,14,0,0,0},     FN_T, ST_CHOIR,      "viadd9"    },
      {  9, 4, {0,3,7,9,0,0,0},      FN_T, ST_MODAL,      "vim6"      },
      {  9, 5, {0,3,7,10,14,0,0},    FN_T, ST_JAZZ,       "vim9"      },
      {  9, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "vim11"     },
      {  9, 4, {0,3,7,11,0,0,0},     FN_T, ST_MODAL,      "vim(maj7)" },
    },
    // row 3 = V (dominant, function D, root_offset = 7)
    {
      {  7, 3, {0,4,7,0,0,0,0},      FN_D, ST_POP,        "V"         },
      {  7, 4, {0,5,7,10,0,0,0},     FN_D, ST_ELECTRONIC, "V7sus4"    },
      {  7, 4, {0,4,7,10,0,0,0},     FN_D, ST_JAZZ,       "V7"        },
      {  7, 5, {0,4,7,10,14,0,0},    FN_D, ST_JAZZ,       "V9"        },
      {  7, 6, {0,4,7,10,14,21,0},   FN_D, ST_JAZZ,       "V13"       },
      {  7, 5, {0,4,7,10,15,0,0},    FN_D, ST_JAZZ,       "V7#9"      },
      {  7, 5, {0,4,7,10,13,0,0},    FN_D, ST_JAZZ,       "V7b9"      },
      {  7, 6, {0,4,7,10,13,15,0},   FN_D, ST_JAZZ,       "V7alt"     },
    },
    // row 4 = IV (subdominant, function PD, root_offset = 5)
    {
      {  5, 3, {0,4,7,0,0,0,0},      FN_PD, ST_POP,       "IV"        },
      {  5, 3, {0,2,7,0,0,0,0},      FN_PD, ST_CHOIR,     "IVsus2"    },
      {  5, 4, {0,4,7,11,0,0,0},     FN_PD, ST_JAZZ,      "IVmaj7"    },
      {  5, 4, {0,4,7,14,0,0,0},     FN_PD, ST_CHOIR,     "IVadd9"    },
      {  5, 4, {0,4,7,9,0,0,0},      FN_PD, ST_CHOIR,     "IV6"       },
      {  5, 5, {0,4,7,11,14,0,0},    FN_PD, ST_JAZZ,      "IVmaj9"    },
      {  5, 5, {0,4,7,9,14,0,0},     FN_PD, ST_CHOIR,     "IV6/9"     },
      {  5, 5, {0,4,7,11,18,0,0},    FN_PD, ST_MODAL,     "IVmaj7#11" },
    },
    // row 5 = iii (mediant, function T, root_offset = 4)
    {
      {  4, 3, {0,3,7,0,0,0,0},      FN_T, ST_POP,        "iii"       },
      {  4, 3, {0,5,7,0,0,0,0},      FN_T, ST_CHOIR,      "iiisus4"   },
      {  4, 4, {0,3,7,10,0,0,0},     FN_T, ST_JAZZ,       "iiim7"     },
      {  4, 4, {0,3,7,14,0,0,0},     FN_T, ST_CHOIR,      "iiiadd9"   },
      {  4, 4, {0,1,3,7,0,0,0},      FN_T, ST_MODAL,      "iii(b9)"   },
      {  4, 5, {0,3,7,10,14,0,0},    FN_T, ST_JAZZ,       "iiim9"     },
      {  4, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "iiim11"    },
      {  4, 5, {0,1,3,7,10,0,0},     FN_T, ST_MODAL,      "iiim7(b9)" },
    },
    // row 6 = ii (predominant, function PD, root_offset = 2)
    {
      {  2, 3, {0,3,7,0,0,0,0},      FN_PD, ST_POP,        "ii"       },
      {  2, 3, {0,2,7,0,0,0,0},      FN_PD, ST_CHOIR,      "iisus2"   },
      {  2, 4, {0,3,7,10,0,0,0},     FN_PD, ST_JAZZ,       "iim7"     },
      {  2, 4, {0,3,7,14,0,0,0},     FN_PD, ST_CHOIR,      "iiadd9"   },
      {  2, 4, {0,3,7,9,0,0,0},      FN_PD, ST_MODAL,      "iim6"     },
      {  2, 5, {0,3,7,10,14,0,0},    FN_PD, ST_JAZZ,       "iim9"     },
      {  2, 4, {0,5,7,10,0,0,0},     FN_PD, ST_ELECTRONIC, "iim7sus4" },
      {  2, 4, {0,3,6,10,0,0,0},     FN_PD, ST_JAZZ,       "iim7b5"   },
    },
    // row 7 = I (tonic, function T, root_offset = 0)
    {
      {  0, 3, {0,4,7,0,0,0,0},      FN_T, ST_POP,        "I"         },
      {  0, 3, {0,5,7,0,0,0,0},      FN_T, ST_CHOIR,      "Isus"      },
      {  0, 4, {0,4,7,11,0,0,0},     FN_T, ST_JAZZ,       "Imaj7"     },
      {  0, 4, {0,4,7,14,0,0,0},     FN_T, ST_CHOIR,      "Iadd9"     },
      {  0, 4, {0,4,7,9,0,0,0},      FN_T, ST_CHOIR,      "I6"        },
      {  0, 5, {0,4,7,11,14,0,0},    FN_T, ST_JAZZ,       "Imaj9"     },
      {  0, 5, {0,4,7,9,14,0,0},     FN_T, ST_CHOIR,      "I6/9"      },
      {  0, 4, {0,5,10,15,0,0,0},    FN_T, ST_ELECTRONIC, "Iquartal"  },
    },
  },

  // ============================================================================
  // CHORD_PALETTE_JAZZ — same column scheme, jazz-flavored chord choices.
  // sus4 over sus2, 7-stacked extensions over bare add-tones, richer cols 5-7.
  // ============================================================================
  {
    // row 0 = bVII (modal mixture, function X, root_offset = 10)
    {
      { 10, 3, {0,4,7,0,0,0,0},      FN_X, ST_MODAL,      "bVII"      },
      { 10, 3, {0,5,7,0,0,0,0},      FN_X, ST_MODAL,      "bVIIsus4"  },
      { 10, 4, {0,4,7,10,0,0,0},     FN_X, ST_JAZZ,       "bVII7"     },
      { 10, 5, {0,4,7,10,14,0,0},    FN_X, ST_JAZZ,       "bVII9"     },
      { 10, 4, {0,4,7,9,0,0,0},      FN_X, ST_MODAL,      "bVII6"     },
      { 10, 6, {0,4,7,10,14,21,0},   FN_X, ST_JAZZ,       "bVII13"    },
      { 10, 4, {0,4,7,11,0,0,0},     FN_X, ST_MODAL,      "bVIImaj7"  },
      { 10, 5, {0,4,7,10,18,0,0},    FN_X, ST_JAZZ,       "bVII7#11"  },
    },
    // row 1 = vii deg — diminished exception; same shape as Pop.
    {
      { 11, 3, {0,3,6,0,0,0,0},      FN_D, ST_JAZZ,       "vii°"      },
      { 11, 4, {0,1,3,6,0,0,0},      FN_D, ST_MODAL,      "vii°(b9)"  },
      { 11, 4, {0,3,6,10,0,0,0},     FN_D, ST_JAZZ,       "viiø7"     },
      { 11, 5, {0,3,6,10,14,0,0},    FN_D, ST_JAZZ,       "viiø9"     },
      { 11, 6, {0,3,6,10,14,17,0},   FN_D, ST_JAZZ,       "viiø11"    },
      { 11, 4, {0,3,6,9,0,0,0},      FN_D, ST_JAZZ,       "viio7"     },
      { 11, 5, {0,1,3,6,9,0,0},      FN_D, ST_MODAL,      "viio7(b9)" },
      EMPTY_TEMPLATE,
    },
    // row 2 = vi (submediant, function T, root_offset = 9)
    {
      {  9, 3, {0,3,7,0,0,0,0},      FN_T, ST_JAZZ,       "vi"        },
      {  9, 3, {0,5,7,0,0,0,0},      FN_T, ST_JAZZ,       "visus4"    },
      {  9, 4, {0,3,7,10,0,0,0},     FN_T, ST_JAZZ,       "vim7"      },
      {  9, 5, {0,3,7,10,14,0,0},    FN_T, ST_JAZZ,       "vim9"      },
      {  9, 4, {0,3,7,9,0,0,0},      FN_T, ST_JAZZ,       "vim6"      },
      {  9, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "vim11"     },
      {  9, 4, {0,3,7,11,0,0,0},     FN_T, ST_JAZZ,       "vim(maj7)" },
      {  9, 6, {0,3,7,10,14,21,0},   FN_T, ST_JAZZ,       "vim13"     },
    },
    // row 3 = V — already maximally jazz; same as Pop.
    {
      {  7, 3, {0,4,7,0,0,0,0},      FN_D, ST_JAZZ,       "V"         },
      {  7, 4, {0,5,7,10,0,0,0},     FN_D, ST_JAZZ,       "V7sus4"    },
      {  7, 4, {0,4,7,10,0,0,0},     FN_D, ST_JAZZ,       "V7"        },
      {  7, 5, {0,4,7,10,14,0,0},    FN_D, ST_JAZZ,       "V9"        },
      {  7, 6, {0,4,7,10,14,21,0},   FN_D, ST_JAZZ,       "V13"       },
      {  7, 5, {0,4,7,10,15,0,0},    FN_D, ST_JAZZ,       "V7#9"      },
      {  7, 5, {0,4,7,10,13,0,0},    FN_D, ST_JAZZ,       "V7b9"      },
      {  7, 6, {0,4,7,10,13,15,0},   FN_D, ST_JAZZ,       "V7alt"     },
    },
    // row 4 = IV (subdominant, function PD, root_offset = 5)
    {
      {  5, 3, {0,4,7,0,0,0,0},      FN_PD, ST_JAZZ,      "IV"        },
      {  5, 3, {0,5,7,0,0,0,0},      FN_PD, ST_JAZZ,      "IVsus4"    },
      {  5, 4, {0,4,7,11,0,0,0},     FN_PD, ST_JAZZ,      "IVmaj7"    },
      {  5, 5, {0,4,7,11,14,0,0},    FN_PD, ST_JAZZ,      "IVmaj9"    },
      {  5, 4, {0,4,7,9,0,0,0},      FN_PD, ST_JAZZ,      "IV6"       },
      {  5, 5, {0,4,7,9,14,0,0},     FN_PD, ST_JAZZ,      "IV6/9"     },
      {  5, 6, {0,4,7,11,14,21,0},   FN_PD, ST_JAZZ,      "IVmaj13"   },
      {  5, 5, {0,4,7,11,18,0,0},    FN_PD, ST_JAZZ,      "IVmaj7#11" },
    },
    // row 5 = iii (mediant, function T, root_offset = 4)
    {
      {  4, 3, {0,3,7,0,0,0,0},      FN_T, ST_JAZZ,       "iii"       },
      {  4, 3, {0,5,7,0,0,0,0},      FN_T, ST_JAZZ,       "iiisus4"   },
      {  4, 4, {0,3,7,10,0,0,0},     FN_T, ST_JAZZ,       "iiim7"     },
      {  4, 5, {0,3,7,10,14,0,0},    FN_T, ST_JAZZ,       "iiim9"     },
      {  4, 4, {0,3,7,9,0,0,0},      FN_T, ST_JAZZ,       "iiim6"     },
      {  4, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "iiim11"    },
      {  4, 4, {0,3,7,11,0,0,0},     FN_T, ST_JAZZ,       "iiim(maj7)"},
      {  4, 5, {0,1,3,7,10,0,0},     FN_T, ST_MODAL,      "iiim7(b9)" },
    },
    // row 6 = ii (predominant, function PD, root_offset = 2)
    {
      {  2, 3, {0,3,7,0,0,0,0},      FN_PD, ST_JAZZ,      "ii"        },
      {  2, 3, {0,5,7,0,0,0,0},      FN_PD, ST_JAZZ,      "iisus4"    },
      {  2, 4, {0,3,7,10,0,0,0},     FN_PD, ST_JAZZ,      "iim7"      },
      {  2, 5, {0,3,7,10,14,0,0},    FN_PD, ST_JAZZ,      "iim9"      },
      {  2, 4, {0,3,7,9,0,0,0},      FN_PD, ST_JAZZ,      "iim6"      },
      {  2, 6, {0,3,7,10,14,17,0},   FN_PD, ST_JAZZ,      "iim11"     },
      {  2, 6, {0,3,7,10,14,21,0},   FN_PD, ST_JAZZ,      "iim13"     },
      {  2, 4, {0,3,6,10,0,0,0},     FN_PD, ST_JAZZ,      "iim7b5"    },
    },
    // row 7 = I (tonic, function T, root_offset = 0)
    {
      {  0, 3, {0,4,7,0,0,0,0},      FN_T, ST_JAZZ,       "I"         },
      {  0, 3, {0,5,7,0,0,0,0},      FN_T, ST_JAZZ,       "Isus4"     },
      {  0, 4, {0,4,7,11,0,0,0},     FN_T, ST_JAZZ,       "Imaj7"     },
      {  0, 5, {0,4,7,11,14,0,0},    FN_T, ST_JAZZ,       "Imaj9"     },
      {  0, 4, {0,4,7,9,0,0,0},      FN_T, ST_JAZZ,       "I6"        },
      {  0, 5, {0,4,7,9,14,0,0},     FN_T, ST_JAZZ,       "I6/9"      },
      {  0, 6, {0,4,7,11,14,21,0},   FN_T, ST_JAZZ,       "Imaj13"    },
      {  0, 5, {0,4,7,11,18,0,0},    FN_T, ST_JAZZ,       "Imaj7#11"  },
    },
  },
};

// Pop-progression invariants — bare triads at col 0 of the I/IV/V/vi rows,
// vii° row col 7 empty. Both palettes must preserve these.
// See docs/VOCABULARY.md §Pop-progression sanity check.
static_assert(vocabularies[CHORD_PALETTE_POP ][7][0].interval_count == 3, "Pop:  I row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_POP ][4][0].interval_count == 3, "Pop:  IV row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_POP ][3][0].interval_count == 3, "Pop:  V row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_POP ][2][0].interval_count == 3, "Pop:  vi row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_POP ][1][7].interval_count == 0, "Pop:  vii° row col 7 must be empty");
static_assert(vocabularies[CHORD_PALETTE_JAZZ][7][0].interval_count == 3, "Jazz: I row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_JAZZ][4][0].interval_count == 3, "Jazz: IV row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_JAZZ][3][0].interval_count == 3, "Jazz: V row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_JAZZ][2][0].interval_count == 3, "Jazz: vi row col 0 must be a triad");
static_assert(vocabularies[CHORD_PALETTE_JAZZ][1][7].interval_count == 0, "Jazz: vii° row col 7 must be empty");

inline const ChordTemplate* chord_template_for_cell(uint8_t palette, uint8_t row, uint8_t col) {
  if (palette >= CHORD_PALETTE_COUNT || row >= 8 || col >= 8) return nullptr;
  const ChordTemplate* tpl = &vocabularies[palette][row][col];
  if (tpl->interval_count == 0) return nullptr;
  return tpl;
}

#endif
