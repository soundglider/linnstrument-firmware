# Roadmap

Phase-by-phase implementation history. Each phase was a small flashable
increment with a clear stop condition. Phases 2–9 are **complete**;
Phase 10 was **skipped** (made obsolete by the per-split refactor) and
Phase 11 is the deferred-polish bucket.

For the architecture this implements, see [ARCHITECTURE.md](ARCHITECTURE.md).
For the layout it produces, see [LAYOUT.md](LAYOUT.md). For the chord
data it consumes, see [VOCABULARY.md](VOCABULARY.md). For toolchain and
flashing, see [BUILD.md](BUILD.md).

---

## Phase 0 — Repo init ✓

Set up the dev workspace, vendored the stock firmware sources, started
the design docs. A real GitHub fork of `rogerlinndesign/linnstrument-firmware`.

## Phase 1 — Toolchain + stock flash ✓

Arduino IDE 1.8.x + SAM Boards 1.6.11 installed. Stock firmware built
and flashed to confirm baseline behavior; revert flow rehearsed.

## Phase 2 — Empty scaffolding ✓

Added the six `ls_chord_*.{ino,h}` files. Wired `handleNewTouch_zoneDispatch`
into stock's `handleNewTouch` and `handleTouchRelease`. Stub zone
handlers emit `Serial.print` lines confirming each cell routes to the
correct zone.

**Stop condition met:** every cell press logged the correct zone over
serial.

## Phase 3 — Vocabulary table ✓

`ls_chord_vocab.h` populated with the 64-cell `vocabulary[8][8]` table.
Compile-time `static_assert`s pin the pop-progression triads at col 0
of rows I, IV, V, vi and the empty cell at vii° col 7.

## Phase 4 — Chord trigger (close-position voicing) ✓

`compute_chord_notes` + `chordEngineHandleTouchOn/Off` complete the path
from chord-cell press to MIDI Note On/Off on the chord channel.

**Bug found and fixed during this phase:** stock's note-emission path
runs in `handleXYZupdate` (the per-tick expression handler), not in
`handleNewTouch`. The patch site for "suppress stock chromatic notes"
turned out to be two gates inside `handleXYZupdate` (around the
`prepareNewNote` and `sendNewNote` calls), not the single gate originally
documented.

## Phase 5 — Tonic strip ✓

`tonic_cell_to_pc[2][8]` and `tonicStripHandleTouchOn` implemented; the
tonic strip re-keys the chord grid live.

## Phase 6 — LED scheme: chord grid + tonic strip ✓

`function_color`, `chord_grid_repaint`, `tonic_strip_repaint` painted
the chord-engine zones. Hooked into `paintNormalDisplay` so the paints
run after stock's per-cell paint (overpainting the chord-engine zones).

**Deviation from original spec:** the original plan was 8 discrete
brightness levels per row to encode column tension. The stock LED driver
only supports a 12-entry palette + global PWM (~6 brightness steps,
not per-cell). All cells in a row are therefore the same color; tension
remains audible but is no longer visually encoded.

## Phase 7 — Melody zone ✓ (refactored)

**Refactor:** the original plan had us implement a scale-aware melody
zone with its own note resolution and LED paint. Mid-phase the design
shifted: the melody zone is now **delegated to stock** so users can
configure it freely (scale-aware piano-roll, guitar tuning, isomorphic
intervals, chromatic, MPE expression, anything stock supports).

The chord-engine zone-dispatcher returns `false` for melody cells →
stock handles them. `paintNormalDisplay` lets stock paint the melody
zone normally. `ls_chord_melody.ino` is now an empty stub.

**Side benefit of the refactor:** chord-grid and melody zones are now
**per-split toggleable** in `displayPerSplit` mode (col 1/9 rows 1+2).
This was the natural integration once we stopped trying to own the
melody zone.

## Phase 8 — EEPROM persistence ✗ (implemented, then reverted)

Implementation: added `chord_current_tonic_pc`, `chord_voicing_mode`,
`chord_voice_spread` to `GlobalSettings`; on tonic / mode change,
scheduled a debounced flash write (3 seconds after last touch) via
`storeSettings`.

**Why reverted:** stock's `writeSettingsToFlash` blanks the entire LED
display while the SAM3X flash controller stalls the CPU (~hundreds of
ms — the whole Configuration struct is many KB → many pages → many
write cycles). The user saw this as a visible flash mid-play after
tonic exploration, which was jarring.

**Current state:** the `GlobalSettings` fields and the boot-time
`applyPresetSettings` sync are kept (so loading a preset restores chord-
engine state), but the chord engine never calls `storeSettings` on its
own. Persistence is opt-in via stock's manual preset save UI; the user
chooses when to pay the LED-blank cost.

## Phase 9 — Voicing v2 (parsimonious) + voice spread ✓

`voice_lead(prev_notes, prev_count, target_pcs, pc_count, out_notes)`
implements greedy nearest-PC assignment from previous voices, with
centroid placement for unmatched target PCs. Common tones (same MIDI
value) are held without retrigger.

Bound at reserved cell (sensorCol 13, row 1): toggles `voicing_mode`
between close-position (0) and parsimonious (1). LED feedback:
`COLOR_ORANGE` when on.

**Extra beyond original spec:** `apply_spread` adds a drop-2+4 jazz-
comping post-process with three levels (0 = tight, 1 = drop-2+4 / drop
the bass for 3-voice triads, 2 = drop-2+4 + raise top). Bound at sensorCol 14 row 1;
cycles through the three levels. LED feedback: off / `COLOR_LIME` /
`COLOR_YELLOW`.

The spread is applied only on **initial chord press** (close-position
path), not on `voice_lead`'s output. In parsimonious mode, the first
press bakes in the spread shape and subsequent chord changes voice-lead
to nearest PCs from those positions — the spread shape is inherited
without compounding octave drops.

**Stop condition met:** parsimonious mode produces audibly smoother
voice motion; spread modes produce wider voicings; common tones held
without retrigger.

## Phase 10 — Melody chromatic mode ✗ (obsolete)

Originally: bind a reserved cell to toggle the melody-zone layout
between scale-relative and chromatic. **Obsolete after Phase 7's
refactor** — the melody zone is now stock-controlled, and stock already
supports configuring the layout via per-split settings (row offset,
scale, note lights, transpose). No work needed.

## Phase 11 (deferred) — Polish + edge cases

These items remain deferred. Lift them when v1 is in regular use and
the trade-offs become clearer:

- **Multi-touch chord layering** — lift the monophonic-chord restriction.
  Track held chord cells in a small ring buffer; combine their pitches
  with deduplication; release per-cell.
- **`poly` row resurrection** — an earlier design sketched a polychord
  / slash-chord row (4 templates) that was dropped to make the grid
  uniform 8×8. Could return as a "shift-held" mode where one of the
  unbound reserved cells (sensorCol 15 or 16 row 1) toggles row 1 from
  vii° to poly.
- **MIDI-in development bridge** — accept Note On / CC over USB MIDI
  that map to chord-cell presses or tonic changes. Useful for testing
  progressions without touching the LinnStrument.
- **Tunable Z-threshold per zone** — stock's pressure thresholds are
  tuned for note playing; chord cells might benefit from a higher
  threshold to avoid accidental triggers from a brushing hand.

## Things added beyond the original roadmap

- **Per-split toggles** for chord mode and tonic strip in `displayPerSplit`
  mode. Each split independently enables or disables the chord-engine
  behavior. Default: split active, splitPoint=9, left chord, right tonic.
- **Voice spread** post-process (Phase 9 extension above).
- **`displayMode != displayNormal` guard** so the chord engine stays
  out of the way in settings / calibration / sequencer modes; the
  release dispatcher still runs to clean up held notes in case the user
  enters a menu mid-chord.

## Verification

End-to-end testing on hardware. Tooling:

| Tool                                | Purpose                                                |
|-------------------------------------|--------------------------------------------------------|
| Arduino IDE 1.8 Serial Monitor      | Read `Serial.print` debug; only works in serial mode.  |
| Surge XT (or any synth) on CH 3     | Hear chord triggers.                                   |
| Stock per-split MIDI                | Hear melody / lead on whatever channel the split uses. |
| MIDI-OX / Pocket MIDI / mido CLI    | Inspect raw note-on / off events while debugging.     |
| The LinnStrument LEDs               | Visual confirmation of every state change.             |

**Important serial-vs-MIDI gotcha:** `Serial.print` in code that's
active while the USB endpoint is in MIDI Class mode will be interpreted
by the host as MIDI bytes (via running status), producing stuck notes
in the high octaves. All chord-engine `Serial.print` calls are gated on
`Device.serialMode`; never call `Serial.print` outside that guard.

## Risks / lessons learned

1. **Arduino IDE 1.8.x + SAM 1.6.11 are old.** Stock requires those
   exact versions. Modern macOS / Windows builds sometimes need
   workarounds — driver install, port detection quirks. See
   [BUILD.md](BUILD.md).

2. **The LED-driver palette is indexed, not RGB.** The original design
   doc spec'd RGB function colors with per-cell brightness. Stock
   firmware uses a 12-entry palette and global PWM with no per-cell
   brightness control. The chord-engine palette adapted to use indexed
   colors and dropped per-column brightness — see
   [LAYOUT.md § LED scheme](LAYOUT.md#led-scheme).

3. **`Serial.print` in MIDI mode corrupts the MIDI stream.** See above.
   The first cut of the chord engine had unconditional debug prints
   that produced ghost notes in the user's synth on every chord press.
   Fix: gate every `Serial.print` on `Device.serialMode`.

4. **Greedy parsimonious voicing isn't strictly optimal.** A
   Hungarian-style assignment would minimize total voice movement
   globally; the greedy nearest-PC match used here can pick a worse
   total in edge cases. Acceptable trade-off: close-position is always
   one toggle away, and voice motion is still small.

5. **Voice drift in parsimonious + spread.** Repeated chord changes can
   slowly move the chord's centroid up or down. Mitigation: `apply_spread`
   is now applied only on the initial press; subsequent parsimonious
   changes inherit the spread without compounding. A radical chord
   change can still drift; releasing everything and re-pressing
   re-establishes the spread baseline.

6. **EEPROM writes blank the LEDs.** Reverted Phase 8 auto-save because
   of this. Persistence is now opt-in via stock's manual preset save.

7. **A flashing accident bricks the LinnStrument temporarily.** This is
   a recoverable state — Roger Linn distributes stock binaries. Practise
   the revert flow during Phase 1 so the path is known. See
   [BUILD.md § Reverting to stock firmware](BUILD.md#reverting-to-stock-firmware).
