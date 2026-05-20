# Hardware Layout

The LinnStrument 128 is divided into three logical zones. This document
specifies their geometry, what each cell does, and the LED scheme.

For the firmware structure that consumes these definitions, see
[ARCHITECTURE.md](ARCHITECTURE.md). For the chord data that backs the
left half, see [VOCABULARY.md](VOCABULARY.md).

## Cells and coordinate convention

LinnStrument 128 = **8 rows × 16 columns** of playable pressure-sensitive
cells. The firmware's existing coordinate convention is:

- `row` index `0` = bottom physical row, `7` = top row.
- The leftmost physical column is `sensorCol == 0` — the **command
  column**, reserved by stock for Global Settings, Per-Split, octave/
  transpose, etc. The playable surface uses `sensorCol 1..16`.

The docs below describe the **playable** surface as 16 cols (i.e., 0..15
"local cols"). Internally those are sensor cols 1..16; the dispatcher
shifts by 1.

## Three zones (default layout)

```
                    cols  1  2  3  4  5  6  7  8    9 10 11 12 13 14 15 16
                       ┌─────────────────────────┬─────────────────────────┐
                  row 7│                         │                         │
                  row 6│     CHORD GRID          │      MELODY ZONE         │
                  row 5│     8 rows × 8 cols     │     (stock-controlled)   │
                  row 4│                         │      6 rows × 8 cols     │
                  row 3│   row = role            │                          │
                  row 2│   col = tension         │                          │
                       │                         ├─────────────────────────┤
                  row 1│                         │      TONIC STRIP         │
                  row 0│                         │      2 rows × 8 cols     │
                       └─────────────────────────┴─────────────────────────┘
```

Each zone is **enabled per split** via a toggle in `displayPerSplit`
mode (see [ARCHITECTURE.md § Per-split toggle UI](ARCHITECTURE.md#per-split-toggle-ui)).
The default state on a fresh flash:

- Split active, split point at col 9 (so left split = cols 1..8, right
  split = cols 9..16).
- LEFT split `chordMode` = on → cols 1..8 are the chord grid.
- RIGHT split `chordTonicStrip` = on → cols 9..16 rows 0..1 are the
  tonic strip.
- RIGHT split top 6 rows = stock note-lights (whatever your right-split
  settings dictate — scale-aware, guitar tuning, isomorphic, anything
  stock supports).

A split with neither toggle enabled is pure stock — same note resolution,
same LEDs, same channel.

## Chord grid (left half: cols 1–8, rows 0–7 = 64 cells)

Each row is one scale-degree role; each column is a tension level within
that role.

### Row → role mapping (top to bottom)

| Row | Role  | Function         | Notes                                                 |
|-----|-------|------------------|-------------------------------------------------------|
| 7   | I     | T (tonic)        | Hand rests at the top of the grid.                    |
| 6   | ii    | PD (predominant) |                                                       |
| 5   | iii   | T (mediant)      |                                                       |
| 4   | IV    | PD               |                                                       |
| 3   | V     | D (dominant)     | The strong-function row sits middle.                  |
| 2   | vi    | T (submediant)   |                                                       |
| 1   | vii°  | D                | Leading-tone diminished family; col 7 is empty in v1. |
| 0   | bVII  | X (chromatic)    | Modal-mixture row — deepest harmonic color at bottom. |

### Column → tension mapping

Column 0 is the most-resolved chord of the role (typically the bare
triad); column 7 is the most tense. Per-row populations are 8 except
`vii°` (7 templates → one column dark).

The plain triads sit at column 0 of rows I, IV, V, vi. Playing a 4-chord
pop loop is the column-0 traversal `(0, 7) → (0, 3) → (0, 2) → (0, 4)`
(I → V → vi → IV). This is a tested invariant in
[VOCABULARY.md § Pop-progression sanity check](VOCABULARY.md#pop-progression-sanity-check).

## Tonic strip (right half: cols 9–16, rows 0–1 = 16 cells)

Local columns are referenced as 0..7 within the strip (sensorCol = local
col + 9):

- 12 cells map to the 12 chromatic pitch classes:
  - Row 0, local cols 0..7 → C, C#, D, D#, E, F, F#, G
  - Row 1, local cols 0..3 → G#, A, A#, B
- 4 reserved cells (row 1, local cols 4..7):
  - **Local col 4** (sensorCol 13) → **voicing mode toggle** (close ↔ parsimonious). Lit orange when parsimonious.
  - **Local col 5** (sensorCol 14) → **voice spread cycle** (tight → drop-2+4 → wider → tight). Lit lime at level 1, yellow at level 2. Cycling re-voices the currently-sounding chord with the new spread (audible transition) and resets the voice-leading baseline so subsequent parsimonious chords lead from the new shape instead of inheriting the old one.
  - **Local col 6** (sensorCol 15) → **chord palette cycle** (Pop → Jazz → Pop). Lit blue for Pop, pink for Jazz. Pressing also releases any held chord (templates differ between palettes).
  - **Local col 7** (sensorCol 16) → **latch toggle**. Lit red when on. While latched: chord-cell release no longer sends note-off (the chord keeps ringing), note-on uses a constant velocity of 100, the next chord-cell press does the normal note-off-old / note-on-new diff, and **re-pressing the same chord cell retriggers** (full note-off / note-on, no common-tone hold). Toggling latch off releases the currently-latched chord.

Pressing a tonic-PC cell:
1. Releases any currently-held chord (avoids mid-chord re-key click).
2. Sets `ChordEngineState.current_tonic_pc` and the corresponding
   persisted `Global.chord_current_tonic_pc`.
3. Repaints the tonic strip (highlight moves).

Note that the tonic strip is anchored to the **bottom 2 rows of the
right split** by design (when `Split[RIGHT].chordTonicStrip` is on). The
melody zone (top 6 rows) remains stock-controlled — pressing a tonic
strip cell does not change which notes the melody zone plays (stock
isn't tonic-aware; its scale, transpose, and root are configured in
per-split settings independently).

## Melody zone (right half: cols 9–16, rows 2–7 = 48 cells)

**Stock-controlled.** The chord-grid engine does not handle this zone —
the cells produce notes, LEDs, and per-split expressions exactly as
stock LinnStrument would, according to whatever you've configured in
that split's Per-Split Settings. This means you can use:

- Stock note lights / accent notes (chromatic, in-key, scale presets).
- Stock row offset and tuning (isomorphic, guitar, custom).
- Stock MIDI mode (one channel, channel-per-row, channel-per-note / MPE).
- Stock expression sends (X / Y / Z, pitch bend, timbre, loudness).
- Stock low-row functions (sustain, CC faders, etc.) — though row 0–1
  of the right split is the tonic strip when `chordTonicStrip` is on,
  so low-row features are typically used outside chord-grid mode.

Splits that have neither chord-grid toggle enabled are entirely stock —
useful if you want one split for chord-grid and another for traditional
playing, or vice versa.

## LED scheme

The stock LED driver only takes an indexed palette (12 colors:
`COLOR_OFF`, `COLOR_RED`, `COLOR_YELLOW`, `COLOR_GREEN`, `COLOR_CYAN`,
`COLOR_BLUE`, `COLOR_MAGENTA`, `COLOR_WHITE`, `COLOR_ORANGE`,
`COLOR_LIME`, `COLOR_PINK`) and a `CellDisplay` enum
(`cellOff`, `cellOn`, `cellFastPulse`, `cellSlowPulse`, `cellFocusPulse`).
There's no raw-RGB API and per-cell brightness is fixed PWM (~6 levels
across the surface).

The chord-engine LED palette is therefore constrained to those indices.

### Chord grid

- **Row background color = harmonic function:**
  - T (rows I, iii, vi): `COLOR_CYAN`
  - PD (rows ii, IV): `COLOR_BLUE`
  - D (rows V, vii°): `COLOR_RED`
  - X (row bVII): `COLOR_MAGENTA`
- **Within a row, all cells share the same color.** The original spec
  envisioned 8 discrete brightness levels per row to encode column
  tension, but the stock LED driver doesn't support per-cell brightness
  beyond global PWM. Tension is still audible (col 0 = bare triad, col
  7 = most extended) — it just isn't visually encoded. The position in
  the row carries the tension cue.
- **Currently-sounding chord cell:** `COLOR_WHITE` `cellFastPulse` on
  `LED_LAYER_PLAYED`, composited over the function-color base.
- **Empty cells** (row 1 col 7 = vii° col 7): off.

### Tonic strip

- 12 PC cells: `COLOR_GREEN` `cellOn`.
- Active tonic: `COLOR_WHITE` `cellOn` (overrides the green).
- Reserved cells (row 1 local cols 4..7):
  - Col 4 (voicing toggle): off when close-position, `COLOR_ORANGE` `cellOn` when parsimonious.
  - Col 5 (spread cycle): off at level 0, `COLOR_LIME` at level 1 (drop-2+4), `COLOR_YELLOW` at level 2 (wider).
  - Cols 6, 7: off (unbound).

### Per-split settings indicators

In `displayPerSplit` mode, the chord-engine toggle indicators light up
at:

- Col 1 row 1, col 9 row 1: `Split[side].colorMain` when the current
  split's `chordMode` is on.
- Col 1 row 2, col 9 row 2: `Split[side].colorMain` when the current
  split's `chordTonicStrip` is on.

The "repeat at col 9" pattern means the toggle is reachable from either
hand without crossing the surface.

### Repaint triggers

LED state is recomputed on:

- Boot (`chordEngineInit()` paints both chord-engine zones).
- Every entry into `displayNormal` (stock's `paintNormalDisplay` runs
  the stock per-cell paint, then our `chord_grid_repaint` and
  `tonic_strip_repaint` overpaint the chord-engine zones).
- Every tonic-cell press (the tonic strip repaints to move the white
  highlight).
- Per-split toggle press (full `updateDisplay`).
- Chord cell press / release (sounding-cell `LED_LAYER_PLAYED` updates).
