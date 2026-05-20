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

// Greedy parsimonious voice-leading. Two cases:
//   - prev_count <= pc_count (same / expand): for each previous note (low → high),
//     pick the unclaimed target PC nearest in semitones (within ±6) and emit a
//     note at that distance. Unclaimed target PCs are placed near the centroid
//     of the already-assigned voices.
//   - prev_count > pc_count (contract): iterate *targets* in vocab order; each
//     target picks its nearest unclaimed prev voice. This preserves upper
//     common tones when downsizing (e.g. Imaj9 → V triad keeps G/B/D in place
//     instead of dropping the upper two voices and re-voicing low).
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

  // Contract case: target-driven matching, no centroid placement needed
  // (every target gets a prev voice since prev_count > pc_count).
  if (prev_count > pc_count) {
    boolean used_prev[MAX_CHORD_VOICES];
    for (uint8_t i = 0; i < MAX_CHORD_VOICES; ++i) used_prev[i] = false;

    uint8_t out_count = 0;
    for (uint8_t j = 0; j < pc_count; ++j) {
      int target_pc = (int)target_pcs[j];

      int best_dist = 99;
      int best_idx = -1;
      int best_note = 0;

      for (uint8_t i = 0; i < prev_count; ++i) {
        if (used_prev[i]) continue;
        int prev_pc = (((int)prev_notes[i] % 12) + 12) % 12;
        int diff = target_pc - prev_pc;
        if (diff > 6) diff -= 12;
        if (diff < -6) diff += 12;
        int dist = (diff < 0) ? -diff : diff;
        if (dist < best_dist) {
          best_dist = dist;
          best_idx = i;
          best_note = (int)prev_notes[i] + diff;
        }
      }

      if (best_idx < 0) break;
      used_prev[best_idx] = true;
      if (best_note < 0)   best_note = 0;
      if (best_note > 127) best_note = 127;
      out_notes[out_count++] = (uint8_t)best_note;
    }
    return out_count;
  }

  // Same-count or expand case: prev-driven matching.
  boolean used[MAX_CHORD_VOICES];
  for (uint8_t i = 0; i < MAX_CHORD_VOICES; ++i) used[i] = false;

  uint8_t out_count = 0;

  for (uint8_t i = 0; i < prev_count; ++i) {
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

// Post-process a close-position voicing to spread voices across a wider
// pitch range. Should only be called on the *initial* voicing of a chord
// (not on voice_lead's output) — applying it twice would drop voices
// further on every chord change, drifting them out of range.
//
//   level 0 → no change.
//   level 1 → drop 2+4 voicing (jazz comping). Drop the 2nd-from-top and
//             4th-from-top voices by an octave. For 3-voice triads this
//             degenerates to "drop the bass" (root dropped an octave for an
//             open-triad spread); inner voices are already minimal at 3
//             notes, so dropping the bass is what produces a meaningful
//             spread without inverting the chord.
//   level 2 → drop 2+4 AND raise the top voice by an octave (super-wide,
//             ~2 octaves total span for 4-voice chords). On triads this
//             stacks atop the dropped bass, spanning two octaves.
//
// Input must be sorted ascending (compute_chord_notes guarantees this since
// intervals are ascending in the vocabulary). Output is re-sorted in place.
void apply_spread(uint8_t* notes, uint8_t count, uint8_t level) {
  if (count < 2 || level == 0) return;

  if (level >= 1) {
    if (count >= 4) {
      // drop 2nd-from-top
      if ((int)notes[count - 2] - 12 >= 0) notes[count - 2] -= 12;
      // drop 4th-from-top
      if ((int)notes[count - 4] - 12 >= 0) notes[count - 4] -= 12;
    } else if (count == 3) {
      // 3-voice fallback: drop the bass (root) by an octave for an open-
      // triad spread. Dropping the middle would invert the chord against
      // the bass; dropping the root keeps the chord shape and just widens
      // the bottom interval.
      if ((int)notes[0] - 12 >= 0) notes[0] -= 12;
    }
  }
  if (level >= 2) {
    if ((int)notes[count - 1] + 12 <= 127) notes[count - 1] += 12;
  }

  // Re-sort ascending after the octave shifts.
  for (uint8_t i = 1; i < count; ++i) {
    uint8_t v = notes[i];
    int j = (int)i - 1;
    while (j >= 0 && notes[j] > v) { notes[j+1] = notes[j]; --j; }
    notes[j+1] = v;
  }
}
