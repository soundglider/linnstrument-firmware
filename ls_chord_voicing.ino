/**************************** ls_chord_voicing: close-position + parsimonious ********************
Pure functions that turn a ChordTemplate + current tonic into absolute MIDI notes.

- compute_chord_notes: close-position voicing. Stacks intervals above
  CHORD_BASE_OCTAVE * 12 + tonic + root_offset. Used in v1 (voicing_mode == 0) and
  as the fallback when there's no previous chord to lead from.

- voice_lead: parsimonious voicing. Given the previous chord's MIDI notes and the
  new chord's target pitch classes, greedily assigns each previous voice to the
  nearest unclaimed target PC (within ±6 semitones), then places any remaining
  target PCs near the centroid of the assigned voices. Common tones produce the
  same MIDI note in the output, so the chord engine can hold them without a
  retrigger.

See docs/ARCHITECTURE.md §Pure-function boundaries, §Touch lifecycles: chord cell,
and docs/ROADMAP.md §Phase 9.
**************************************************************************************************/

#include "ls_chord_config.h"
#include "ls_chord_vocab.h"

uint8_t compute_chord_notes(const ChordTemplate* tpl,
                            uint8_t tonic_pc,
                            uint8_t base_octave,
                            uint8_t out_notes[MAX_CHORD_VOICES]) {
  if (tpl == nullptr || tpl->interval_count == 0) return 0;

  uint8_t n = tpl->interval_count;
  if (n > MAX_CHORD_VOICES) n = MAX_CHORD_VOICES;

  int base = (int)base_octave * 12 + (int)tonic_pc + (int)tpl->root_offset;
  for (uint8_t i = 0; i < n; ++i) {
    int note = base + (int)tpl->intervals[i];
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    out_notes[i] = (uint8_t)note;
  }
  return n;
}

// Compute the pitch classes (0..11) of a chord template, accounting for the
// current tonic. Out array must be at least MAX_CHORD_VOICES.
uint8_t compute_chord_pcs(const ChordTemplate* tpl,
                          uint8_t tonic_pc,
                          uint8_t out_pcs[MAX_CHORD_VOICES]) {
  if (tpl == nullptr || tpl->interval_count == 0) return 0;
  uint8_t n = tpl->interval_count;
  if (n > MAX_CHORD_VOICES) n = MAX_CHORD_VOICES;
  for (uint8_t i = 0; i < n; ++i) {
    int v = (int)tonic_pc + (int)tpl->root_offset + (int)tpl->intervals[i];
    out_pcs[i] = (uint8_t)(((v % 12) + 12) % 12);
  }
  return n;
}

// Greedy parsimonious voice-leading. For each previous note (low → high), pick
// the unclaimed target PC nearest in semitones (within ±6) and emit a note at
// that distance. Unclaimed target PCs are placed near the centroid of the
// already-assigned voices.
uint8_t voice_lead(const uint8_t* prev_notes, uint8_t prev_count,
                   const uint8_t* target_pcs, uint8_t pc_count,
                   uint8_t out_notes[MAX_CHORD_VOICES]) {
  if (pc_count == 0) return 0;

  // No previous voices: fall back to a close-position voicing from
  // CHORD_BASE_OCTAVE, stacking ascending.
  if (prev_count == 0) {
    uint8_t out_count = 0;
    int prev = -1;
    for (uint8_t i = 0; i < pc_count && out_count < MAX_CHORD_VOICES; ++i) {
      int n = (int)CHORD_BASE_OCTAVE * 12 + (int)target_pcs[i];
      while (n <= prev) n += 12;
      if (n > 127) n = 127;
      out_notes[out_count++] = (uint8_t)n;
      prev = n;
    }
    return out_count;
  }

  boolean used[MAX_CHORD_VOICES];
  for (uint8_t i = 0; i < MAX_CHORD_VOICES; ++i) used[i] = false;

  uint8_t out_count = 0;
  uint8_t assign_limit = (prev_count < pc_count) ? prev_count : pc_count;

  for (uint8_t i = 0; i < assign_limit; ++i) {
    int prev_note = (int)prev_notes[i];
    int prev_pc = ((prev_note % 12) + 12) % 12;

    int best_dist = 99;
    int best_idx = -1;
    int best_note = 0;

    for (uint8_t j = 0; j < pc_count; ++j) {
      if (used[j]) continue;
      int diff = (int)target_pcs[j] - prev_pc;
      if (diff > 6) diff -= 12;
      if (diff < -6) diff += 12;
      int dist = (diff < 0) ? -diff : diff;
      if (dist < best_dist) {
        best_dist = dist;
        best_idx = j;
        best_note = prev_note + diff;
      }
    }

    if (best_idx < 0) break;
    used[best_idx] = true;
    if (best_note < 0)   best_note = 0;
    if (best_note > 127) best_note = 127;
    out_notes[out_count++] = (uint8_t)best_note;
  }

  // Place any unclaimed target PCs near the centroid of the assigned voices.
  if (out_count < pc_count && out_count > 0) {
    int sum = 0;
    for (uint8_t i = 0; i < out_count; ++i) sum += out_notes[i];
    int centroid = sum / out_count;
    int centroid_pc = ((centroid % 12) + 12) % 12;

    for (uint8_t j = 0; j < pc_count && out_count < MAX_CHORD_VOICES; ++j) {
      if (used[j]) continue;
      int diff = (int)target_pcs[j] - centroid_pc;
      if (diff > 6) diff -= 12;
      if (diff < -6) diff += 12;
      int candidate = centroid + diff;
      if (candidate < 0)   candidate = 0;
      if (candidate > 127) candidate = 127;
      used[j] = true;
      out_notes[out_count++] = (uint8_t)candidate;
    }
  }

  return out_count;
}

// Post-process voicing to spread voices across a wider pitch range.
//
//   level 0 → no change (tight close-position or parsimonious)
//   level 1 → drop the lowest voice by an octave IF it's currently at or above
//             CHORD_BASE_OCTAVE*12 (so the bass sits in the octave below the
//             chord cluster). The threshold prevents unbounded downward drift
//             across long chord progressions in parsimonious mode — once the
//             bass is already in the spread zone, subsequent chord changes
//             leave it alone.
//   level 2 → same as level 1, plus raise the highest voice by an octave if
//             it's currently at or below (CHORD_BASE_OCTAVE+1)*12.
//
// Common-tone preservation: voices that are already in their spread position
// stay put across chord changes (no retrigger). Only voices that *cross* the
// threshold are octave-shifted, which costs at most a one-time retrigger.
void apply_spread(uint8_t* notes, uint8_t count, uint8_t level) {
  if (count < 2 || level == 0) return;

  // Insertion-sort ascending (small N).
  for (uint8_t i = 1; i < count; ++i) {
    uint8_t v = notes[i];
    int j = (int)i - 1;
    while (j >= 0 && notes[j] > v) { notes[j+1] = notes[j]; --j; }
    notes[j+1] = v;
  }

  const int bass_threshold = (int)CHORD_BASE_OCTAVE * 12;
  const int top_threshold  = ((int)CHORD_BASE_OCTAVE + 1) * 12;

  if (level >= 1) {
    if ((int)notes[0] >= bass_threshold && (int)notes[0] - 12 >= 0) {
      notes[0] = (uint8_t)((int)notes[0] - 12);
    }
  }
  if (level >= 2 && count >= 3) {
    uint8_t top = notes[count - 1];
    if ((int)top <= top_threshold && (int)top + 12 <= 127) {
      notes[count - 1] = (uint8_t)((int)top + 12);
    }
  }

  // Re-sort in case the bass/top shift reordered voices.
  for (uint8_t i = 1; i < count; ++i) {
    uint8_t v = notes[i];
    int j = (int)i - 1;
    while (j >= 0 && notes[j] > v) { notes[j+1] = notes[j]; --j; }
    notes[j+1] = v;
  }
}
