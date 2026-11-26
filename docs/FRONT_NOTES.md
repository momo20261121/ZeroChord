# Front notes

Press `Tab` and open **App → Front notes**. Press `Tab` for the next tab, or
`1`–`8` for **All** and the tabs for `Z`–`M`. Use `F` / `X` (↑ / ↓) to select a
row. `Z` / `C` (← / →) change Strum time and Octave at once, with a preview.
`Enter` opens the list of choices for the other rows. `Esc` cancels or goes back. Dedicated arrow keys also work.

In a list, moving to a choice previews one front note. Press `Enter` to
save the choice. `Backspace` restores the highlighted field, or highlights its
default in a list.

## All keys

- **Chord notes.** Front keys follow the selected chord's root and quality.
  Defaults preserve the original minichord-inspired seven-note layout.
- **Scale notes.** `Z`–`M` play degrees I–VII starting at octave 4. **Key**
  and **Scale** follow **Chord config → All** by default. Choosing either sets
  the front's own key and scale together. `Backspace` follows the chord again.
  The notes can play before you choose a backing chord. Holding `1`
  raises new scale notes by one semitone, including custom notes and octaves.
- **Scale.** **Follow chord** uses the Chord config scale. The front's own
  scale can be Major, Natural minor, Major pentatonic, Minor pentatonic, Blues,
  Harmonic minor, Melodic minor, Dorian, Mixolydian, Lydian, or Phrygian.
  Pentatonic scales have five notes and Blues six, fewer than the seven keys,
  so the remaining keys continue an octave up: with a pentatonic scale, `N` and
  `M` play notes 1 and 2 an octave higher; with Blues, `M` plays note 1 an
  octave higher. A key set to a fixed Octave stays in that octave.
- **Sharpen chord notes.** Holding `1` raises new front notes one semitone above the
  selected chord's notes, including when that chord was already sharp. The
  backing chord stays unchanged. Ringing notes and queued strums keep their
  original pitches.
- **K/L action.** **Chord notes** defaults to **Strum** and **Scale notes**
  defaults to **Octave shift**. Each mode remembers its own action.
- **Strum time:** 60–600 ms from first to last note, shown when Strum is selected.
  Each mode remembers its own duration. `L` plays `Z` → `M`, and `K` plays
  `M` → `Z`, preserving
  keyboard order even with a nonascending custom layout.
- **Restore all front:** resets both modes, all keys, K/L actions and timings,
  and makes Key and Scale follow the chord again.

Each front note sounds while you hold its key. Letting go starts the front
envelope's **Release**, so keep it short for crisp notes or longer to let them ring.
Strums play notes of a fixed length.

**Octave shift** moves front notes down with `K` or up with `L`. It leaves the backing
unchanged and stops before any note would exceed MIDI 0–127. The live shift is
separate for each mode and is not saved into per-key mappings.

## Each key

- **Note, in Chord notes.** Choose **Auto note** for the original chord-based
  layout, one of four chord tones, or a fixed interval of **0–11 semitones** above
  the chord root. Chord tones follow the selected chord quality. A fixed +4 is
  always a major third above the root. The example shows the resulting pitch.
- **Note, in Scale notes:** choose degree I–VII. The shown interval follows the
  selected scale. For a pentatonic or Blues scale, the choices read **Note 1**,
  **Note 2**, and so on, and those past the scale's last note add **up**, such
  as **Note 1 up**, for the next octave.
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

Open **App → Chord config**. Press `Tab` to move between **All** and the seven
degree tabs. `1` selects **All**. `2`–`8` select II–VII and I, in the same order
as on the playing keyboard. `Z` / `C` (← / →) change Octave and Chord combo
timing; `Enter` opens the choices for the other rows.

**All → Chord combo timing** groups chord-button presses within a fixed interval
from the first press. The default is **30 ms**. Choose **Off** or **5–120 ms**
in 5 ms steps. A longer window tolerates more finger stagger but adds the same
amount of backing response delay, on top of keyboard/audio latency. This is not
a measured optimum for this device. Front notes respond immediately using the
latest chord selection. Releases do not retrigger partial combinations. Panic
cancels a pending chord. Resetting All restores timing, Hold, and mappings.

Chord config begins with Octave on each tab. All contains Key, Scale, and Hold.
**Scale** places the degree roots in one of eight seven-note scales: Major,
Natural minor, Harmonic minor, Melodic minor, Dorian, Mixolydian, Lydian, or
Phrygian. Each choice lists its roots in the current key; in Natural minor, III,
VI, and VII sit a semitone lower. Pentatonic and Blues are front-only, because
the chord grid needs seven roots. Degree tabs contain absolute Root overrides.
Follow Key / Follow All restores inheritance.

### Choose a chord quality

On **All** or a degree tab, select the row for the buttons you want to combine.
Press `Enter`, choose a quality, then press `Enter` again. A degree can use its
own quality or **Follow All** for the shared choice.

| Quality | What it plays | Example from C |
| --- | --- | --- |
| **no3** | Root and fifth | C, G |
| **sus2** | Second in place of the third | C, D, G |
| **sus4** | Fourth in place of the third | C, F, G |
