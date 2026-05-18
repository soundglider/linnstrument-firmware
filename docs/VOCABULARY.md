# Chord Vocabulary

The hand-authored chord table that backs the 8 × 8 chord grid. Each
cell is one `ChordTemplate`; the firmware looks up `vocabulary[row][col]`
at touch time and emits the chord's MIDI notes transposed by the
current tonic.

For the physical layout the table maps onto, see [LAYOUT.md](LAYOUT.md).
For how the engine consumes the table, see
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

struct ChordTemplate {
    uint8_t      root_offset;       // 0..11 semitones above tonic
    uint8_t      interval_count;    // 3..7 (0 = empty / no-op cell)
    uint8_t      intervals[7];      // semitones above the chord's root
    uint8_t      function;          // FN_T / FN_PD / FN_D / FN_X
    uint8_t      style;             // ST_* (cosmetic; see LAYOUT.md)
    const char*  name;              // short label (serial debug / future display)
};

// 8 rows × 8 columns; row 7 is the top of the grid (I), row 0 the bottom (bVII).
extern const ChordTemplate vocabulary[8][8] PROGMEM;

#define EMPTY_TEMPLATE { 0, 0, {0,0,0,0,0,0,0}, FN_T, ST_POP, "" }
```

## How the row index works

The row index uses the **physical row coordinate** (row 0 = bottom,
row 7 = top), so `vocabulary[7]` is the I row and `vocabulary[0]` is
the bVII row. The tables below are written **top-down** to match the
visual grid; in the C++ array literal they appear in reverse order
(row 7 last, row 0 first).

## Row 7 — I (tonic) [function T]

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

## Row 6 — ii (predominant) [function PD]

| Col | Name       | Root | Intervals                          | Style |
|-----|------------|------|------------------------------------|-------|
| 0   | ii         |  2   | 0 · 3 · 7                          | pop   |
| 1   | iisus2     |  2   | 0 · 2 · 7                          | choir |
| 2   | iim7sus4   |  2   | 0 · 5 · 7 · 10                     | elect |
| 3   | iim7       |  2   | 0 · 3 · 7 · 10                     | jazz  |
| 4   | iiadd9     |  2   | 0 · 3 · 7 · 14                     | choir |
| 5   | iim9       |  2   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 6   | iim6       |  2   | 0 · 3 · 7 · 9                      | modal |
| 7   | iim7b5     |  2   | 0 · 3 · 6 · 10                     | jazz  |

## Row 5 — iii (mediant) [function T]

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | iii         |  4   | 0 · 3 · 7                          | pop   |
| 1   | iiisus4     |  4   | 0 · 5 · 7                          | choir |
| 2   | iiim7       |  4   | 0 · 3 · 7 · 10                     | jazz  |
| 3   | iiiadd9     |  4   | 0 · 3 · 7 · 14                     | choir |
| 4   | iii(b9)     |  4   | 0 · 1 · 3 · 7                      | modal |
| 5   | iiim9       |  4   | 0 · 3 · 7 · 10 · 14                | jazz  |
| 6   | iiim7(b9)   |  4   | 0 · 1 · 3 · 7 · 10                 | modal |
| 7   | iiim11      |  4   | 0 · 3 · 7 · 10 · 14 · 17           | jazz  |

## Row 4 — IV (subdominant) [function PD]

| Col | Name        | Root | Intervals                          | Style |
|-----|-------------|------|------------------------------------|-------|
| 0   | IV          |  5   | 0 · 4 · 7                          | pop   |
| 1   | IVsus2      |  5   | 0 · 2 · 7                          | choir |
| 2   | IVmaj7      |  5   | 0 · 4 · 7 · 11                     | jazz  |
| 3   | IVadd9      |  5   | 0 · 4 · 7 · 14                     | choir |
| 4   | IVmaj9      |  5   | 0 · 4 · 7 · 11 · 14                | jazz  |
| 5   | IV6         |  5   | 0 · 4 · 7 · 9                      | choir |
| 6   | IV6/9       |  5   | 0 · 4 · 7 · 9 · 14                 | choir |
| 7   | IVmaj7#11   |  5   | 0 · 4 · 7 · 11 · 18                | modal |

## Row 3 — V (dominant) [function D]

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

## Row 2 — vi (submediant) [function T]

| Col | Name        | Root | Intervals                            | Style |
|-----|-------------|------|--------------------------------------|-------|
| 0   | vi          |  9   | 0 · 3 · 7                            | pop   |
| 1   | vim7        |  9   | 0 · 3 · 7 · 10                       | jazz  |
| 2   | visus2      |  9   | 0 · 2 · 7                            | choir |
| 3   | viadd9      |  9   | 0 · 3 · 7 · 14                       | choir |
| 4   | vim9        |  9   | 0 · 3 · 7 · 10 · 14                  | jazz  |
| 5   | vim11       |  9   | 0 · 3 · 7 · 10 · 14 · 17             | jazz  |
| 6   | vim(maj7)   |  9   | 0 · 3 · 7 · 11                       | modal |
| 7   | vim6        |  9   | 0 · 3 · 7 · 9                        | modal |

## Row 1 — vii° (leading-tone) [function D]

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

## Row 0 — bVII (modal mixture) [function X]

| Col | Name        | Root | Intervals                            | Style |
|-----|-------------|------|--------------------------------------|-------|
| 0   | bVII        | 10   | 0 · 4 · 7                            | modal |
| 1   | bVIIsus2    | 10   | 0 · 2 · 7                            | modal |
| 2   | bVIIadd9    | 10   | 0 · 4 · 7 · 14                       | modal |
| 3   | bVII6       | 10   | 0 · 4 · 7 · 9                        | modal |
| 4   | bVIImaj7    | 10   | 0 · 4 · 7 · 11                       | modal |
| 5   | bVII7sus4   | 10   | 0 · 5 · 7 · 10                       | modal |
| 6   | bVII7       | 10   | 0 · 4 · 7 · 10                       | modal |
| 7   | bVII9       | 10   | 0 · 4 · 7 · 10 · 14                  | jazz  |

## Pop-progression sanity check

Plain triads must always sit at column 0 of the I, IV, V, and vi rows
(rows 7, 4, 3, 2). The user can then tap a 4-chord pop loop
**I → V → vi → IV** by hitting cells:

```
(col 0, row 7)  →  (col 0, row 3)  →  (col 0, row 2)  →  (col 0, row 4)
```

A compile-time `static_assert` in `ls_chord_vocab.h` pins this:

```c++
static_assert(vocabulary[7][0].interval_count == 3, "I row col 0 must be a triad");
static_assert(vocabulary[4][0].interval_count == 3, "IV row col 0 must be a triad");
static_assert(vocabulary[3][0].interval_count == 3, "V row col 0 must be a triad");
static_assert(vocabulary[2][0].interval_count == 3, "vi row col 0 must be a triad");
static_assert(vocabulary[1][7].interval_count == 0, "vii° row col 7 must be empty");
```

(`static_assert` works on `constexpr` arrays — keep the vocabulary
`constexpr` or use `_Static_assert` at file scope.)

## Tension ordering rationale

Columns are tension-ascending within each row, where "tension" is
loosely the perceptual distance from the stable triad — a triad-first
bias so plain triads always sit at col 0, then progressively more
colored extensions toward col 7. The ordering is **frozen at authoring
time** — the firmware does no live re-sort, so a future vocabulary edit
must re-verify the column order manually.

Sketch of the criteria used for each row:

1. Pure triads first.
2. Then sus / add-tone variants (sus2 / sus4 / add9 / 6).
3. Then jazz 7-extended (7, m7, maj7).
4. Then extended (9, 11, 13).
5. Then altered / colored (b9, #9, alt, #11) and half-/full-diminished
   at the right end.

When manual judgement is needed (e.g. is `iim6` more or less tense than
`iim9`?), pick the one that *feels* more colored — what reads as
"further from the bare triad's repose."

## Adding a chord later

Each row is an independent block of 8 cells in the C++ table. To add a
new chord:

1. Pick the row (its role) and decide where in tension order it goes.
2. Shift the existing cells to the right; drop the rightmost.
3. (Or replace an empty cell, e.g. row 1 col 7.)
4. Rebuild + flash.

The grid is therefore **append-friendly within a row** and
**role-fixed across rows**. No re-ID step is needed (the firmware
indexes by `(row, col)`, not by template_id).
