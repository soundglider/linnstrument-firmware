LinnStrument — chord-grid fork
==============================

This is a fork of [`rogerlinndesign/linnstrument-firmware`](https://github.com/rogerlinndesign/linnstrument-firmware)
that adds a **chord-grid mode** to the LinnStrument 128: a self-contained
chord-progression instrument with role × tension chord cells, a tonic
selector, and parsimonious voice-leading — no host computer needed.

The stock firmware sources are kept verbatim; the chord-grid logic lives
in six new `ls_chord_*.{ino,h}` files plus a handful of small targeted
patches to existing stock files.

These sources assume that you're using Arduino IDE v1.8.1 with SAM boards
v1.6.11. Different versions of this package might create unknown build and
execution problems.

Bug reports or feature requests for the **stock firmware** belong with
Roger Linn: support@rogerlinndesign.com. Issues with the **chord-grid
additions** belong on this fork's GitHub Issues.


## What the fork adds

A per-split toggle (configured in **Per-Split Settings**) turns each of
the LinnStrument's two splits into one of these layouts:

- **Chord grid** (8 cols × 8 rows). Each row is one scale-degree role —
  I, ii, iii, IV, V, vi, vii°, bVII. Columns follow a role-locked scheme:
  col 1 = triad, col 2 = sus, col 3 = 7th chord, cols 4-7 add function-
  aware extensions, col 8 = altered wildcard. Press a cell to sound that
  chord. 64 hand-authored chord templates per palette.

- **Tonic strip** (8 cols × 2 rows). The 12 chromatic pitch classes are
  laid out as a tonic selector; pressing one re-keys the chord grid live.
  Four reserved cells bind to: **voicing mode** (close-position ↔
  parsimonious), **voice spread** (tight / drop-2+4 / wide), **chord
  palette** (Pop ↔ Jazz), and **latch** (on/off — when on, a chord cell
  release no longer sends note-off; the chord keeps ringing until the
  next chord-cell press or until latch is toggled off).

Two chord palettes ship in firmware: **Pop** (default, friendly diatonic
voicings) and **Jazz** (sus4 over sus2, 7-stacked extensions, maj13 /
m11 / m(maj7) / lydian-dominant wildcards). The column scheme is the same
in both — switching palette swaps the specific voicings without changing
which column holds the sus / 7 / extension.

A split with neither toggle enabled behaves exactly like stock — same
note resolution, same note-lights, same per-split MIDI channel and
expression settings.

**Default layout on a fresh flash:** split active with split point at
col 9. Left split = chord grid. Right split bottom 2 rows = tonic strip.
Right split top 6 rows = stock note-lights (your normal LinnStrument).
Chord notes go out on MIDI channel 3; tonic strip presses don't send MIDI
themselves (they change which key the chord grid plays in); melody / lead
zones use whatever channel and layout your per-split settings specify.


## Voicing

Two voicing modes (toggled at the voicing reserved cell):

- **Close position** — the chord template's intervals stacked above
  `CHORD_BASE_OCTAVE` + tonic. Each chord press starts fresh; common tones
  between chords are not deliberately preserved.
- **Parsimonious** — `voice_lead` matches the previous chord's voices to
  the new chord's pitch classes within ±6 semitones; common tones (same
  MIDI value) are held without retrigger. Voices move minimally between
  chord changes. The previous voicing **survives release**, so playing
  chords one at a time (release before next press) still voice-leads —
  reset only on tonic change, palette change, or preset load. When the
  new chord has fewer voices than the previous one (e.g. Imaj9 → V
  triad), upper common tones are preserved instead of dropped.

A separate **voice spread** parameter (3 levels at the spread reserved
cell) applies a drop-2+4 jazz-comping post-process to the *initial*
voicing of a chord. The spread shape is then inherited via parsimonious
voice-leading on subsequent chord changes, so the chord stays in roughly
the same pitch range without compounding octave drops.


## Where to read more

The chord-grid design docs live in [`docs/`](docs/):

| Doc                                       | What's in it                                                  |
|-------------------------------------------|---------------------------------------------------------------|
| [`docs/LAYOUT.md`](docs/LAYOUT.md)        | Physical layout: zones, cells, LED scheme, palette            |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Firmware structure: state, zone dispatch, touch lifecycles |
| [`docs/VOCABULARY.md`](docs/VOCABULARY.md)| The 64-cell chord table (hand-authored C++ data)              |
| [`docs/BUILD.md`](docs/BUILD.md)          | Toolchain, build steps, flashing, reverting to stock          |
| [`docs/ROADMAP.md`](docs/ROADMAP.md)      | Phase ladder + implementation history                          |


## Where the chord-grid lives in this tree

| File                       | Role                                                |
|----------------------------|-----------------------------------------------------|
| `ls_chord_config.h`        | Constants + `ChordEngineState` struct + tonic-strip PC map |
| `ls_chord_vocab.h`         | 64-entry chord vocabulary + `chord_template_for_cell` |
| `ls_chord_engine.ino`      | Runtime state + zone dispatch + chord/tonic touch handlers |
| `ls_chord_voicing.ino`     | `compute_chord_notes`, `voice_lead`, `apply_spread` |
| `ls_chord_leds.ino`        | LED painters for the chord grid and tonic strip     |
| `ls_chord_melody.ino`      | Empty stub (melody zone is stock-delegated)         |

Patched stock files (small, additive hooks — original behavior preserved
outside the chord-engine zones):

| File                        | Patch                                                       |
|-----------------------------|-------------------------------------------------------------|
| `linnstrument-firmware.ino` | `setup()` calls `chordEngineInit()`; `GlobalSettings` and `SplitSettings` gain a few chord-engine fields |
| `ls_handleTouches.ino`      | Zone dispatch in `handleNewTouch` / `handleTouchRelease`; chord-engine cells suppress stock note resolution in `handleXYZupdate` |
| `ls_displayModes.ino`       | `paintNormalDisplay` overpaints chord-engine zones; `paintPerSplitDisplay` shows the chord-mode and tonic-strip toggle indicators |
| `ls_settings.ino`           | Default values for new fields; preset sync in `applyPresetSettings`; press handling for the per-split toggle cells |

Hardware drivers — touch detection, LED driver, MIDI send, EEPROM
framework, calibration, switches, sensors, sequencer — are **untouched**.
