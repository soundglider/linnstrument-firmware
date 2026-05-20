# Chord Vocabulary

The hand-authored chord tables that back the 8 × 8 chord grid. Two palettes
ship in firmware: **Pop** (default) and **Jazz**. Each cell is one
`ChordTemplate`; the firmware looks up
`vocabularies[chord_engine_state.chord_palette][row][col]` at touch time and
emits the chord's MIDI notes transposed by the current tonic.

For the physical layout the tables map onto, see [LAYOUT.md](LAYOUT.md).
For how the engine consumes the tables, see
[ARCHITECTURE.md § Touch lifecycles](ARCHITECTURE.md#touch-lifecycles).

## Data structure

`ls_chord_vocab.h`:

```c++
// Function enum
#define FN_T   0
#define FN_PD  1
#define FN_D   2
#define FN_X   3

// Style enum (LED-stripe hint, may or may not be used in LED scheme)
#define ST_POP        0
#define ST_CHOIR      1
#define ST_JAZZ       2
#define ST_ELECTRONIC 3
#define ST_MODAL      4

// Palette index
enum ChordPaletteId : uint8_t {
    CHORD_PALETTE_POP   = 0,
    CHORD_PALETTE_JAZZ  = 1,
    CHORD_PALETTE_COUNT = 2,
};

struct ChordTemplate {
    uint8_t      root_offset;       // 0..11 semitones above tonic
    uint8_t      interval_count;    // 3..7 (0 = empty / no-op cell)
    uint8_t      intervals[7];      // semitones above the chord's root
    uint8_t      function;          // FN_T / FN_PD / FN_D / FN_X
    uint8_t      style;             // ST_* (cosmetic; see LAYOUT.md)
    const char*  name;              // short label (serial debug / future display)
};

// 2 palettes × 8 rows × 8 columns. Row 7 = top of the grid (I), row 0 = bVII.
extern const ChordTemplate vocabularies[CHORD_PALETTE_COUNT][8][8] PROGMEM;

#define EMPTY_TEMPLATE { 0, 0, {0,0,0,0,0,0,0}, FN_T, ST_POP, "" }
```

## How the row index works

The row index uses the **physical row coordinate** (row 0 = bottom,
row 7 = top), so `vocabularies[palette][7]` is the I row and
`vocabularies[palette][0]` is the bVII row. The tables below are written
**top-down** to match the visual grid; in the C++ array literal they appear
in reverse order (row 7 last, row 0 first).

## Column scheme

Columns are role-locked across all rows so the player builds muscle memory
across the grid:

| Local col | Meaning            | Notes                                       |
|-----------|--------------------|---------------------------------------------|
| 0         | **Triad**          | Foundation. Pinned by `static_assert` for I/IV/V/vi rows. |
| 1         | **Sus**            | sus2 in Pop (open), sus4 in Jazz (tense). vii° has no natural sus and reuses col 1 for `vii°(b9)` (the first alteration); V uses `V7sus4` (the canonical dominant sus). |
| 2         | **7th chord**      | Characteristic 7 of the function: maj7 (I, IV), m7 (ii, iii, vi), dom7 (V, bVII), ø7 (vii°). |
| 3         | +9 / 9             | Pop: `add9` (no 7). Jazz: `9` (= 7 + 9). Dominant rows always use `9` here. |
| 4         | 6                  | bare 6 (or m6 for minor rows). iii uses `iii(b9)` as a color-tone substitute (no natural iii6). Dominant rows use `13` here. |
| 5         | Higher extension   | Pop: `9` (full extended). Jazz: `6/9` or `11`. |
| 6         | Highest extension  | Pop: `6/9` or `11`. Jazz: `13` or `m(maj7)`. |
| 7         | Altered / wildcard | `maj7#11`, `m7b5`, `m(maj7)`, `quartal`, `m7(b9)`, etc. vii° row col 7 must be empty (invariant). |

Cols 0 and 2 are also the LED-highlight cells (RED for the I/IV/V rows and
WHITE for the rest — see [LAYOUT.md](LAYOUT.md)), so the visual highlight
is structurally meaningful: triad + 7th chord = the two anchor voicings.

### Function exceptions

| Row | Function | Deviation |
|-----|----------|-----------|
| 3 (V)    | Dominant      | Col 1 = `V7sus4` (dom7sus is the canonical sus form). Cols 3-7 use 9 / 13 / #9 / b9 / alt instead of the diatonic ascent. |
| 1 (vii°) | Diminished    | No natural sus exists. Col 1 = `vii°(b9)`. Col 7 stays empty per the `static_assert` invariant. |

---

## Pop palette

### Row 7 — I (tonic) [function T]

| Col | Name      | Root | Intervals (semi above root)       | Style      |
|-----|-----------|------|-----------------------------------|------------|
| 0   | I         |  0   | 0 · 4 · 7                         | pop        |
| 1   | Isus      |  0   | 0 · 5 · 7                         | choir      |
| 2   | Imaj7     |  0   | 0 · 4 · 7 · 11                    | jazz       |
| 3   | Iadd9     |  0   | 0 · 4 · 7 · 14                    | choir      |
| 4   | I6        |  0   | 0 · 4 · 7 · 9                     | choir      |
| 5   | Imaj9     |  0   | 0 · 4 · 7 · 11 · 14               | jazz       |
| 6   | I6/9      |  0   | 0 · 4 · 7 · 9 · 14                | choir      |
| 7   | Iquartal  |  0   | 0 · 5 · 10 · 15                   | electronic |

### Row 6 — ii (predominant) [function PD]

| Col | Name       | Root | Intervals                          | Style |
|-----|------------|------|------------------------------------|-------|
| 0   | ii         |  2   | 0 · 3 · 7                          | pop   |
| 1   | iisus2     |  2   | 0 · 2 · 7                          | choir |
| 2   | iim7       |  2   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | iiadd9     |  2   | 0 · 3 · 7 · 14                     | choir |
| 4   | iim6       |  2   | 0 · 3 · 7 · 9                      | modal |
| 5   | iim9       |  2   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 6   | iim7sus4   |  2   | 0 · 5 · 7 · 10                     | elect |
| 7   | iim7b5     |  2   | 0 · 3 · 6 · 10                     | jazz  |

### Row 5 — iii (mediant) [function T]

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | iii         |  4   | 0 · 3 · 7                          | pop   |
| 1   | iiisus4     |  4   | 0 · 5 · 7                          | choir |
| 2   | iiim7       |  4   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | iiiadd9     |  4   | 0 · 3 · 7 · 14                     | choir |
| 4   | iii(b9)     |  4   | 0 · 1 · 3 · 7                      | modal |
| 5   | iiim9       |  4   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 6   | iiim11      |  4   | 0 · 3 · 7 · 10 · 14 · 17           | jazz  |
| 7   | iiim7(b9)   |  4   | 0 · 1 · 3 · 7 · 10                 | modal |

### Row 4 — IV (subdominant) [function PD]

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | IV          |  5   | 0 · 4 · 7                          | pop   |
| 1   | IVsus2      |  5   | 0 · 2 · 7                          | choir |
| 2   | IVmaj7      |  5   | 0 · 4 · 7 · 11                     | jazz  |
| 3   | IVadd9      |  5   | 0 · 4 · 7 · 14                     | choir |
| 4   | IV6         |  5   | 0 · 4 · 7 · 9                      | choir |
| 5   | IVmaj9      |  5   | 0 · 4 · 7 · 11 · 14                | jazz  |
| 6   | IV6/9       |  5   | 0 · 4 · 7 · 9 · 14                 | choir |
| 7   | IVmaj7#11   |  5   | 0 · 4 · 7 · 11 · 18                | modal |

### Row 3 — V (dominant) [function D]

| Col | Name      | Root | Intervals                            | Style |
|-----|-----------|------|--------------------------------------|-------|
| 0   | V         |  7   | 0 · 4 · 7                            | pop   |
| 1   | V7sus4    |  7   | 0 · 5 · 7 · 10                       | elect |
| 2   | V7        |  7   | 0 · 4 · 7 · 10                       | jazz  |
| 3   | V9        |  7   | 0 · 4 · 7 · 10 · 14                  | jazz  |
| 4   | V13       |  7   | 0 · 4 · 7 · 10 · 14 · 21             | jazz  |
| 5   | V7#9      |  7   | 0 · 4 · 7 · 10 · 15                  | jazz  |
| 6   | V7b9      |  7   | 0 · 4 · 7 · 10 · 13                  | jazz  |
| 7   | V7alt     |  7   | 0 · 4 · 7 · 10 · 13 · 15             | jazz  |

### Row 2 — vi (submediant) [function T]

| Col | Name        | Root | Intervals                            | Style |
|-----|-------------|------|--------------------------------------|-------|
| 0   | vi          |  9   | 0 · 3 · 7                            | pop   |
| 1   | visus2      |  9   | 0 · 2 · 7                            | choir |
| 2   | vim7        |  9   | 0 · 3 · 7 · 10                       | jazz  |
| 3   | viadd9      |  9   | 0 · 3 · 7 · 14                       | choir |
| 4   | vim6        |  9   | 0 · 3 · 7 · 9                        | modal |
| 5   | vim9        |  9   | 0 · 3 · 7 · 10 · 14                  | jazz  |
| 6   | vim11       |  9   | 0 · 3 · 7 · 10 · 14 · 17             | jazz  |
| 7   | vim(maj7)   |  9   | 0 · 3 · 7 · 11                       | modal |

### Row 1 — vii° (leading-tone) [function D]

| Col | Name        | Root | Intervals                            | Style |
|-----|-------------|------|--------------------------------------|-------|
| 0   | vii°        | 11   | 0 · 3 · 6                            | jazz  |
| 1   | vii°(b9)    | 11   | 0 · 1 · 3 · 6                        | modal |
| 2   | viiø7       | 11   | 0 · 3 · 6 · 10                       | jazz  |
| 3   | viiø9       | 11   | 0 · 3 · 6 · 10 · 14                  | jazz  |
| 4   | viiø11      | 11   | 0 · 3 · 6 · 10 · 14 · 17             | jazz  |
| 5   | viio7       | 11   | 0 · 3 · 6 · 9                        | jazz  |
| 6   | viio7(b9)   | 11   | 0 · 1 · 3 · 6 · 9                    | modal |
| 7   | *(empty)*   |  —   |                                     | —    |

### Row 0 — bVII (modal mixture) [function X]

| Col | Name        | Root | Intervals                            | Style |
|-----|-------------|------|--------------------------------------|-------|
| 0   | bVII        | 10   | 0 · 4 · 7                            | modal |
| 1   | bVIIsus2    | 10   | 0 · 2 · 7                            | modal |
| 2   | bVII7       | 10   | 0 · 4 · 7 · 10                       | modal |
| 3   | bVIIadd9    | 10   | 0 · 4 · 7 · 14                       | modal |
| 4   | bVII6       | 10   | 0 · 4 · 7 · 9                        | modal |
| 5   | bVII9       | 10   | 0 · 4 · 7 · 10 · 14                  | jazz  |
| 6   | bVII7sus4   | 10   | 0 · 5 · 7 · 10                       | modal |
| 7   | bVIImaj7    | 10   | 0 · 4 · 7 · 11                       | modal |

---

## Jazz palette

Same column scheme. sus4 replaces sus2 at col 1, extensions stack on the 7
(`Imaj9` instead of `Iadd9` at col 3), and cols 5-7 use richer voicings
(maj13, m11, m(maj7), lydian-dominant). Diatonic functions go through the
same diatonic order; V and vii° are unchanged from Pop because their pop
shapes are already canonically jazz.

### Row 7 — I (jazz)

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | I           |  0   | 0 · 4 · 7                          | jazz  |
| 1   | Isus4       |  0   | 0 · 5 · 7                          | jazz  |
| 2   | Imaj7       |  0   | 0 · 4 · 7 · 11                     | jazz  |
| 3   | Imaj9       |  0   | 0 · 4 · 7 · 11 · 14                | jazz  |
| 4   | I6          |  0   | 0 · 4 · 7 · 9                      | jazz  |
| 5   | I6/9        |  0   | 0 · 4 · 7 · 9 · 14                 | jazz  |
| 6   | Imaj13      |  0   | 0 · 4 · 7 · 11 · 14 · 21           | jazz  |
| 7   | Imaj7#11    |  0   | 0 · 4 · 7 · 11 · 18                | jazz  |

### Row 6 — ii (jazz)

| Col | Name     | Root | Intervals                          | Style |
|-----|----------|------|------------------------------------|-------|
| 0   | ii       |  2   | 0 · 3 · 7                          | jazz  |
| 1   | iisus4   |  2   | 0 · 5 · 7                          | jazz  |
| 2   | iim7     |  2   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | iim9     |  2   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 4   | iim6     |  2   | 0 · 3 · 7 · 9                      | jazz  |
| 5   | iim11    |  2   | 0 · 3 · 7 · 10 · 14 · 17           | jazz  |
| 6   | iim13    |  2   | 0 · 3 · 7 · 10 · 14 · 21           | jazz  |
| 7   | iim7b5   |  2   | 0 · 3 · 6 · 10                     | jazz  |

### Row 5 — iii (jazz)

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | iii         |  4   | 0 · 3 · 7                          | jazz  |
| 1   | iiisus4     |  4   | 0 · 5 · 7                          | jazz  |
| 2   | iiim7       |  4   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | iiim9       |  4   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 4   | iiim6       |  4   | 0 · 3 · 7 · 9                      | jazz  |
| 5   | iiim11      |  4   | 0 · 3 · 7 · 10 · 14 · 17           | jazz  |
| 6   | iiim(maj7)  |  4   | 0 · 3 · 7 · 11                     | jazz  |
| 7   | iiim7(b9)   |  4   | 0 · 1 · 3 · 7 · 10                 | modal |

### Row 4 — IV (jazz)

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | IV          |  5   | 0 · 4 · 7                          | jazz  |
| 1   | IVsus4      |  5   | 0 · 5 · 7                          | jazz  |
| 2   | IVmaj7      |  5   | 0 · 4 · 7 · 11                     | jazz  |
| 3   | IVmaj9      |  5   | 0 · 4 · 7 · 11 · 14                | jazz  |
| 4   | IV6         |  5   | 0 · 4 · 7 · 9                      | jazz  |
| 5   | IV6/9       |  5   | 0 · 4 · 7 · 9 · 14                 | jazz  |
| 6   | IVmaj13     |  5   | 0 · 4 · 7 · 11 · 14 · 21           | jazz  |
| 7   | IVmaj7#11   |  5   | 0 · 4 · 7 · 11 · 18                | jazz  |

### Row 3 — V (jazz — same shape as Pop V)

Same as Pop row 3. The dominant row is already maximally jazz; no
differentiation is needed.

### Row 2 — vi (jazz)

| Col | Name       | Root | Intervals                          | Style |
|-----|------------|------|------------------------------------|-------|
| 0   | vi         |  9   | 0 · 3 · 7                          | jazz  |
| 1   | visus4     |  9   | 0 · 5 · 7                          | jazz  |
| 2   | vim7       |  9   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | vim9       |  9   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 4   | vim6       |  9   | 0 · 3 · 7 · 9                      | jazz  |
| 5   | vim11      |  9   | 0 · 3 · 7 · 10 · 14 · 17           | jazz  |
| 6   | vim(maj7)  |  9   | 0 · 3 · 7 · 11                     | jazz  |
| 7   | vim13      |  9   | 0 · 3 · 7 · 10 · 14 · 21           | jazz  |

### Row 1 — vii° (jazz — same shape as Pop vii°)

Same as Pop row 1. Half- and fully-diminished colors are already jazz
canonical.

### Row 0 — bVII (jazz)

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | bVII        | 10   | 0 · 4 · 7                          | modal |
| 1   | bVIIsus4    | 10   | 0 · 5 · 7                          | modal |
| 2   | bVII7       | 10   | 0 · 4 · 7 · 10                     | jazz  |
| 3   | bVII9       | 10   | 0 · 4 · 7 · 10 · 14                | jazz  |
| 4   | bVII6       | 10   | 0 · 4 · 7 · 9                      | modal |
| 5   | bVII13      | 10   | 0 · 4 · 7 · 10 · 14 · 21           | jazz  |
| 6   | bVIImaj7    | 10   | 0 · 4 · 7 · 11                     | modal |
| 7   | bVII7#11    | 10   | 0 · 4 · 7 · 10 · 18                | jazz  |

---

## Pop-progression sanity check

Plain triads must always sit at column 0 of the I, IV, V, and vi rows
(rows 7, 4, 3, 2) **in both palettes**. The user can then tap a 4-chord
pop loop **I → V → vi → IV** by hitting cells:

```
(col 0, row 7)  →  (col 0, row 3)  →  (col 0, row 2)  →  (col 0, row 4)
```

Compile-time `static_assert`s in `ls_chord_vocab.h` pin this for both
palettes:

```c++
static_assert(vocabularies[CHORD_PALETTE_POP ][7][0].interval_count == 3, ...);
static_assert(vocabularies[CHORD_PALETTE_POP ][4][0].interval_count == 3, ...);
static_assert(vocabularies[CHORD_PALETTE_POP ][3][0].interval_count == 3, ...);
static_assert(vocabularies[CHORD_PALETTE_POP ][2][0].interval_count == 3, ...);
static_assert(vocabularies[CHORD_PALETTE_POP ][1][7].interval_count == 0, ...);
static_assert(vocabularies[CHORD_PALETTE_JAZZ][7][0].interval_count == 3, ...);
// ...same set repeated for Jazz.
```

## Adding a chord later

Each `(palette, row)` block is an independent 8-cell row in the C++ table.
To add a new chord:

1. Pick the palette, the row (its role), and the column it should occupy
   per the column scheme above.
2. Replace the existing cell at that `(palette, row, col)`. If you need to
   add rather than replace, shift cells right and drop the rightmost, or
   replace an empty cell (e.g. row 1 col 7).
3. Rebuild + flash.

The grid is therefore **column-fixed across rows and palettes** (each
column has a stable role) and **palette-flexible within a cell** (you can
swap the specific voicing per palette without affecting the column scheme).
No re-ID step is needed — the firmware indexes by `(palette, row, col)`,
not by template_id.
