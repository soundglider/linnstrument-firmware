/**************************** ls_chord_vocab: 8x8 chord vocabulary table *************************
The hand-authored 64-cell chord table. Row index uses the physical row convention
(row 0 = bottom = bVII, row 7 = top = I), so the C++ literal below appears in reverse
order vs. the top-down tables in docs/VOCABULARY.md.

See docs/VOCABULARY.md for the source-of-truth listing and the rationale behind
column tension ordering.
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

struct ChordTemplate {
  uint8_t      root_offset;
  uint8_t      interval_count;
  uint8_t      intervals[7];
  uint8_t      function;
  uint8_t      style;
  const char*  name;
};

#define EMPTY_TEMPLATE { 0, 0, {0,0,0,0,0,0,0}, FN_T, ST_POP, "" }

constexpr ChordTemplate vocabulary[8][8] = {
  // row 0 = bVII (modal mixture, function X, root_offset = 10)
  {
    { 10, 3, {0,4,7,0,0,0,0},      FN_X, ST_MODAL,      "bVII"      },
    { 10, 3, {0,2,7,0,0,0,0},      FN_X, ST_MODAL,      "bVIIsus2"  },
    { 10, 4, {0,4,7,14,0,0,0},     FN_X, ST_MODAL,      "bVIIadd9"  },
    { 10, 4, {0,4,7,9,0,0,0},      FN_X, ST_MODAL,      "bVII6"     },
    { 10, 4, {0,4,7,11,0,0,0},     FN_X, ST_MODAL,      "bVIImaj7"  },
    { 10, 4, {0,5,7,10,0,0,0},     FN_X, ST_MODAL,      "bVII7sus4" },
    { 10, 4, {0,4,7,10,0,0,0},     FN_X, ST_MODAL,      "bVII7"     },
    { 10, 5, {0,4,7,10,14,0,0},    FN_X, ST_JAZZ,       "bVII9"     },
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
    {  9, 4, {0,3,7,10,0,0,0},     FN_T, ST_JAZZ,       "vim7"      },
    {  9, 3, {0,2,7,0,0,0,0},      FN_T, ST_CHOIR,      "visus2"    },
    {  9, 4, {0,3,7,14,0,0,0},     FN_T, ST_CHOIR,      "viadd9"    },
    {  9, 5, {0,3,7,10,14,0,0},    FN_T, ST_JAZZ,       "vim9"      },
    {  9, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "vim11"     },
    {  9, 4, {0,3,7,11,0,0,0},     FN_T, ST_MODAL,      "vim(maj7)" },
    {  9, 4, {0,3,7,9,0,0,0},      FN_T, ST_MODAL,      "vim6"      },
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
    {  5, 5, {0,4,7,11,14,0,0},    FN_PD, ST_JAZZ,      "IVmaj9"    },
    {  5, 4, {0,4,7,9,0,0,0},      FN_PD, ST_CHOIR,     "IV6"       },
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
    {  4, 5, {0,1,3,7,10,0,0},     FN_T, ST_MODAL,      "iiim7(b9)" },
    {  4, 6, {0,3,7,10,14,17,0},   FN_T, ST_JAZZ,       "iiim11"    },
  },
  // row 6 = ii (predominant, function PD, root_offset = 2)
  {
    {  2, 3, {0,3,7,0,0,0,0},      FN_PD, ST_POP,        "ii"       },
    {  2, 3, {0,2,7,0,0,0,0},      FN_PD, ST_CHOIR,      "iisus2"   },
    {  2, 4, {0,5,7,10,0,0,0},     FN_PD, ST_ELECTRONIC, "iim7sus4" },
    {  2, 4, {0,3,7,10,0,0,0},     FN_PD, ST_JAZZ,       "iim7"     },
    {  2, 4, {0,3,7,14,0,0,0},     FN_PD, ST_CHOIR,      "iiadd9"   },
    {  2, 5, {0,3,7,10,14,0,0},    FN_PD, ST_JAZZ,       "iim9"     },
    {  2, 4, {0,3,7,9,0,0,0},      FN_PD, ST_MODAL,      "iim6"     },
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
};

// Pop-progression invariants — pin the bare triads at column 0 of the I/IV/V/vi rows
// and the single empty cell at vii° col 7. See docs/VOCABULARY.md §Pop-progression sanity check.
static_assert(vocabulary[7][0].interval_count == 3, "I row col 0 must be a triad");
static_assert(vocabulary[4][0].interval_count == 3, "IV row col 0 must be a triad");
static_assert(vocabulary[3][0].interval_count == 3, "V row col 0 must be a triad");
static_assert(vocabulary[2][0].interval_count == 3, "vi row col 0 must be a triad");
static_assert(vocabulary[1][7].interval_count == 0, "vii° row col 7 must be empty");

inline const ChordTemplate* chord_template_for_cell(uint8_t row, uint8_t col) {
  if (row >= 8 || col >= 8) return nullptr;
  const ChordTemplate* tpl = &vocabulary[row][col];
  if (tpl->interval_count == 0) return nullptr;
  return tpl;
}

#endif
