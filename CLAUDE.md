# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A fork of `rogerlinndesign/linnstrument-firmware` that adds a **chord-grid mode** to
the LinnStrument 128 hardware controller. The stock firmware sources are kept
verbatim at the repo root; all new behavior lives in six `ls_chord_*.{ino,h}`
files plus small targeted patches to existing stock files. See
[`README.md`](README.md) for the user-facing description.

The detailed design docs in [`docs/`](docs/) are the authoritative reference —
prefer reading them over inferring from code:

| Doc | Use it when |
|-----|-------------|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Changing zone dispatch, voicing pure functions, touch lifecycles, settings sync, LED repaint flow |
| [`docs/LAYOUT.md`](docs/LAYOUT.md) | Anything involving cell coordinates, the three zones, LED palette, per-split toggle UI |
| [`docs/VOCABULARY.md`](docs/VOCABULARY.md) | Editing chord templates in `ls_chord_vocab.h`; understand the static_assert invariants before changing row 0 col |
| [`docs/BUILD.md`](docs/BUILD.md) | Toolchain, flashing, reverting to stock, serial debug |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | What's been built, what was tried-and-reverted (Phase 8 EEPROM auto-save), what's deferred |

## Build and flash

There is **no command-line build**. This is an Arduino sketch.

- Open `linnstrument-firmware.ino` in **Arduino IDE 1.8.x** (not 2.x).
- Board: **Arduino Due (Native USB Port)**, via **SAM Boards package 1.6.11**.
  Newer SAM packages have not been validated.
- Compile: Ctrl+R. Upload: Ctrl+U (flashes via `bossac` over the silver native
  USB port, not the programming port).
- All `.ino` files in the sketch directory link automatically — there's no
  Makefile and no manifest to update when adding a file.
- `.vscode/arduino.json` is configured for the vscode-arduino extension if you
  prefer that over the IDE.

There are **no unit tests, no linters, no CI**. Verification is on hardware
(serial monitor at 115200 baud, a synth listening on MIDI channel 3, plus a
MIDI monitor). See [`docs/BUILD.md`](docs/BUILD.md) and
[`docs/ROADMAP.md` § Verification](docs/ROADMAP.md#verification).

## Code architecture — high-level

**Runtime model.** Arduino single-threaded loop. ISRs only touch the sensor
matrix; engine state lives in `ChordEngineState chord_engine_state` (defined
in `ls_chord_engine.ino`, declared in `ls_chord_config.h`) and is mutated only
by zone handlers.

**Zone dispatch is the central seam.** `handleNewTouch_zoneDispatch(col, row, vel)`
(in `ls_chord_engine.ino`) is called from stock's `handleNewTouch` and
`handleTouchRelease`. It returns `true` to consume the touch in the chord
engine, `false` to let stock handle it. The predicate
`isChordEngineZone(col, row)` is what every other site gates on.

```
left split (cols 1-8)              right split (cols 9-16)
┌─────────────────────┐    ┌─────────────────────────────────┐
│                     │    │   MELODY ZONE (rows 2-7)        │
│   CHORD GRID        │    │   stock-controlled              │
│   8×8               │    ├─────────────────────────────────┤
│   row=role,col=tens │    │   TONIC STRIP (rows 0-1)        │
└─────────────────────┘    └─────────────────────────────────┘
```

Zones are gated by **per-split toggles** in `SplitSettings`
(`chordMode`, `chordTonicStrip`), set via the Per-Split Settings UI. A split
with neither toggle is pure stock.

**Pure-function voicing layer.** `ls_chord_voicing.ino` exposes
`compute_chord_notes`, `compute_chord_pcs`, `voice_lead`, `apply_spread` — all
pure. Mutation happens only in zone handlers. If you ever build off-target
tests, this is the seam.

**Stock files patched, not replaced.** Patches in `linnstrument-firmware.ino`,
`ls_handleTouches.ino`, `ls_displayModes.ino`, `ls_settings.ino` are small and
additive — see [`docs/ARCHITECTURE.md` § What we modify](docs/ARCHITECTURE.md).
The note-emission suppression patch sits inside `handleXYZupdate` (the
per-tick expression handler), **not** in `handleNewTouch`; stock emits notes
from the update tick.

## Non-obvious things that have already bitten us

These come from `docs/ROADMAP.md` § Risks/lessons learned. Re-reading them
before edits in the relevant area saves a flash-cycle round trip.

1. **`Serial.print` corrupts MIDI when USB is in MIDI Class mode.** The host
   reinterprets serial bytes as MIDI via running status → ghost notes in high
   octaves. Every chord-engine `Serial.print` must be gated on
   `Device.serialMode`. Don't add unconditional prints.

2. **EEPROM writes blank the LED display.** Stock's `writeSettingsToFlash`
   stalls the CPU for hundreds of ms while the SAM3X flash controller works,
   and LEDs are blanked during that window. Phase 8's auto-save was reverted
   for this reason. The chord engine never calls `storeSettings` /
   `writeSettingsToFlash`; persistence is opt-in via stock's manual preset
   save UI. The `GlobalSettings`/`SplitSettings` fields and the
   `applyPresetSettings` sync are kept so loading a preset restores state.

3. **LED palette is indexed (12 entries) with global PWM, not per-cell RGB.**
   Original spec'd per-column brightness for tension; not possible. Tension
   is audible (col 0 = bare triad, col 7 = most extended), not visually
   encoded — don't try to "fix" this.

4. **Release dispatcher runs even when `chordMode` is off / in settings
   modes.** The chord-cell off-handler is intentionally always invoked; it
   self-gates on `sounding_cell_*`. This cleans up state if the user enters a
   menu mid-chord-hold or toggles chord mode off while held. Don't add a
   `displayMode == displayNormal` guard around release like there is for press.

5. **`sensorCol == 0` is the command column.** The playable surface is cols
   1..16; the dispatcher subtracts 1 to get local cols.

6. **Vocabulary invariants are pinned by `static_assert`.** Plain triads must
   stay at col 0 of rows I, IV, V, vi; row 1 col 7 must stay empty. The
   4-chord pop loop test (`I→V→vi→IV`) depends on this. See
   [`docs/VOCABULARY.md` § Pop-progression sanity check](docs/VOCABULARY.md#pop-progression-sanity-check).

7. **`apply_spread` runs only on the initial close-position press**, not on
   `voice_lead` output. The spread shape is then inherited via parsimonious
   voice-leading, so the chord doesn't drift octave-down on every change.

## Conventions

- New chord-engine code goes in `ls_chord_*` files. Existing stock files
  get only small targeted patches so a future merge from upstream stays
  tractable.
- `ChordEngineState` lives in the header (`ls_chord_config.h`) so the LED
  painter can read it without externs.
- Compile-time tunables (zone geometry, MIDI channels, base octaves) go in
  `ls_chord_config.h`, not scattered as magic numbers.
