# Front notes

Open **Tab → App → Front notes**. **←/→** switches **All, Z, X, C, V,
B, N, M** tabs. **↑/↓** selects a row; **Enter** opens its choices and
confirms; **Esc** cancels or goes back. The printed arrows **F/X/Z/C** work on
every page and in choice lists, as do dedicated arrow keys. Z–M key-selection
shortcuts are disabled; Z/X/C perform navigation, rather than jumping to a key.
Numbers do not select front-key tabs.
**Backspace** restores the highlighted field. Choice changes audition one front
note without saving until Enter.

## All keys

- **Front mode — Chord notes:** keys follow the selected chord's root and quality.
  Defaults preserve the original minichord-inspired seven-note layout.
- **Front mode — Scale notes:** Z–M play I–VII of Chord config → All → Key and the front Scale setting, starting
  at octave 4. They work without selecting a backing chord. Selecting a backing
  chord does not transpose these scale notes; holding **1** raises newly played
  scale notes by one semitone, including custom notes and octaves.
- **1 in Chord notes:** holding 1 raises new front notes one semitone above the
  selected chord's notes, including when that chord was already sharp. The
  backing chord stays unchanged. Ringing notes and queued strums keep their
  original pitches.
- **K/L action:** Strum or Octave shift, remembered separately for each front
  mode. Chord notes defaults to Strum; Scale notes defaults to Octave shift.
- **Strum time:** 60–600 ms from first to last note, shown when Strum is selected.
  Each mode remembers its own duration. L plays Z→M, K plays M→Z, preserving
  keyboard order even with a nonascending custom layout.
- **Restore all front:** resets both modes, all keys, K/L actions and timings.

Octave shift moves front notes down with K or up with L. It leaves the backing
unchanged and stops before any note would exceed MIDI 0–127. The live shift is
separate for each mode and is not saved into per-key mappings.

## Each key

- **Note, in Chord notes.** Choose **Auto note** for the original chord-based
  layout, one of four chord tones, or a fixed interval of **0–11 semitones** above
  the chord root. Chord tones follow the selected chord quality. A fixed +4 is
  always a major third above the root. The example shows the resulting pitch.
- **Note, in Scale notes:** choose degree I–VII. The shown interval follows the
  selected major or natural-minor scale.
- **Octave.** An octave of 4 means C4–B4. **Auto octave (4)** shows that the
  original layout puts this example pitch in octave 4. Set a specific octave to
  choose where the note plays.
- **Restore this key:** restores the selected key in the current front mode.

The example pitch uses a major chord on the configured I root in Chord notes
mode, or the selected key and scale in Scale notes mode. Performance pitches
also reflect the live K/L octave shift. Z–M, their on-screen pitches, and strums
share the same mapping.

All front mappings and actions are app-wide, persist on clean exit, and survive
preset changes/restores. Front sound remains a style setting. Older offset/range
settings migrate to actual note octaves and intervals using the selected tonic;
untouched defaults retain Auto.

## Chord config

Open **App → Chord config**. ←/→ switches All and the seven degree
tabs; 1 selects All and 2–8 selects II–VII/I as on the playing keyboard.

**All → Chord combo timing** groups chord-button presses within a fixed interval
from the first press. The trial default is **30 ms**; choose **Off** or **5–60 ms**
in 5 ms steps. A longer window tolerates more finger stagger but adds the same
amount of backing response delay, on top of keyboard/audio latency. This is not
a measured optimum for this device. Front notes respond immediately using the
latest chord selection. Releases do not retrigger partial combinations. Panic
cancels a pending chord. Resetting All restores timing as well as mappings;
resetting an individual degree preserves the shared timing.

Chord config begins with Octave on each tab. All contains Key; degree tabs
contain absolute Root overrides. Follow Key / Follow All restores inheritance.
Major/minor is now selected under Front notes → All → Scale, visible only in
Scale notes mode, and has no effect on backing roots.
