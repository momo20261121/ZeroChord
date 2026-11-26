# Release notes

## 1.3.0 - 2026-11-25

- Hold a front note to sustain it. Its key stays lit, and letting go starts the
  envelope's release.
- **Backing layers** is one page with Layers, Chord, Arp, and Pad tabs. On
  Layers, Enter mutes or unmutes a layer and left/right set its mix, both while
  playing. Each layer tab holds its pattern, sound, and envelope.
- Arp has its own sound and envelope. Older setups keep Arp on the Chord sound.
- Built-in presets select all three layers. Layers a preset did not use before
  join at 30%. Unedited built-ins upgrade; edited presets keep their layers.
- Settings use the same keys on every page: Tab for the next tab, 1-8 to jump,
  left/right to change numbers (hold to repeat), Enter to open rows marked `>`,
  Backspace to restore, Esc to go back. Each row explains itself on the line below.
- Envelope lists show the sound's own value in order and mark the preset default.
- Restore and save actions ask once: Enter to confirm, Esc to keep.
- Chord config adds Scale for natural-minor chord roots. Front scale notes
  follow the chord key and scale by default, or use their own.
- A preview stops when you leave its option.

## 1.2.1 - 2026-11-24

- Adjust volume, Echo, and EQ directly with the left and right arrows under
  App → Volume & effects. System Vol controls the device output when available.
- Find Chord config before Front notes. Move Hold into Chord config → All and
  extend Chord combo timing to 120 ms.
- Shorten the store description and link to the user guide after the quick start.

## 1.2.0 - 2026-11-22

- Group Master, Backing and Front volume, Echo, and EQ under App → Volume & effects.
- Make setup hints specific to the current page and action. Show the scope under
  the active tab and list backing layers in Chord, Arp, Pad order.
- Separate chord keys, Roman numerals, and chord qualities on the performance
  screen. Give Key and Preset their own labeled fields.
- Keep picker help text clear of the last visible row and refresh the launcher icon.
- Refresh the user guide and screenshots for the updated controls.

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
