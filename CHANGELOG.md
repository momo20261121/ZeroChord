# Release notes

## 1.1.0 — 2026-11-22

- Add an About settings tab with the running version, a GitHub QR code and a pointer to the user guide and details in the repository.

- Add an optional pinned, MIT-licensed Grand Piano (FluidR3) pack and an offline
  matched-level audio comparison tool with native-renderer reference checks.

- O and Style → Backing layers share a checkbox popup: combine Chord, Pad and
  Arp, or clear all for Front only. Enter toggles; Apply saves; Esc cancels.
- Dream starts with Chord, Pad and Arp enabled; factory Manual hit defaults
  become Sustained.
- Sound pickers show source sample rates, including mixed-rate instruments
  and imported sounds. This is metadata, not a sound-quality rating.
- Retune all nine factory presets with distinct sounds, envelopes, tempo and
  accompaniment. Preserve customized defaults and per-preset edits on upgrade.
- Add Every eighth and Bossa chord patterns, lower bass voicing for Waltz/Bossa,
  descriptive arp names and a regular Up/down turnaround; retain saved IDs.
- Preserve native Chord/Arp decay/sustain and use flowing gates for regular arps.
  Independent backing layers share tempo; Pad motion affects only Pad.

- Add Steel-string Guitar, Clean Electric Guitar, Overdriven Guitar, and Celesta
  to the named sound choices, using the existing runtime bank.
- The main page shows an active +1 hint and chord-grid labels while holding 1.
  Selected sharp chords remain unchanged on release.
- 1 sharpens new front notes in both modes independently of backing selection;
  front-note labels match the pitches to be played.

## 1.0.0

First packaged release of ZeroChord for CardputerZero.

- Seven chord columns with major, minor, seventh, and customizable combinations.
- Separate Front, Chord, and Pad sounds with independent envelopes.
- Pad, Chord, Arp, and Front only modes; one-shot, sustained, and rhythmic chords.
- Nine editable presets, remembered changes, individual defaults and current-preset restore.
- Chord or scale front notes, per-key notes/octaves, and K/L strum or octave shift.
- Global volume, three-band EQ, Echo, Hold, chord roots/octaves and combination timing.
- Dark/light themes and settings operated with the Cardputer's printed arrows.
- Named SoundFont packs loaded from a TF card.

Saved prototype settings are retained. Startup is silent; select a chord to play.
Settings save on a clean exit. The runtime sound bank is supplied separately by
the `timgm6mb-soundfont` Debian package; it is not bundled or relicensed.
