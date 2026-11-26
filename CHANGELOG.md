# Release notes

## 1.4.0 (in development)

- New **Bass** and **Drums** layers. Both play on the beat clock while a chord
  sounds (Hold keeps them going; Space stops them). Bass follows the chord's root,
  or its slash bass, an octave or two below, with its own sound, pattern and
  envelope. Drums play a General MIDI kit and ignore the chord.
- Backing layers has Layers, Chord, Arp, Pad, Bass and Drums tabs. Layers lists all
  five with mute/unmute and mix. Bass has Pattern (Root notes, Held root, Root and
  fifth, Driving eighths, Octave pulse, Offbeat, Syncopated, Bossa nova, Waltz 3/4),
  Sound and Envelope. Drums has Kit (Standard, Room, Power, Electronic, TR-808,
  Jazz, Brush, Orchestra, plus pack kits) and Pattern (Basic beat, Pop, Rock, R&B
  groove, Half-time, Four on the floor, Bossa nova, Waltz 3/4, Brushes, Chimes).
- **Swing** on the Tempo page (`P`, or Style → Tempo), which now has three rows:
  Tempo (type digits, Enter applies), Swing (Off, 8ths or 16ths; Enter or
  left/right cycles) and Swing amount (left/right, 0–100% in 5% steps, default 50%).
  Swing and Swing amount apply at once; Swing amount is dimmed while Swing is Off. 0% is
  straight and 100% a full triplet feel. 8ths swing delays the "and" of each beat
  (its 16ths move in proportion); 16ths swing delays every second 16th. Every
  style starts with swing Off. R loads the preset tempo and swing; Esc keeps swing
  changes and drops a typed tempo you did not apply. The Tempo row reads, for
  example, **100 BPM, 8ths swing 60%**, and the `P` hint adds **swing** when it is on.
- New mixing engine. Layer mixes are independent faders: 100% is full level and
  50% about -12 dB, and layers no longer share a level budget. Each melodic sound
  is level-matched once, the first time it is used, against the built-in Piano
  (within -14 to +12 dB), so a mix means the same whatever sound a layer plays.
  Output is about 15 dB louder, with a look-ahead limiter at -1 dBFS. Each part
  plays on its own bus with a pan position and sends to a stereo reverb, whose
  decay time each style sets, and to the Echo. The Echo now repeats on a dotted
  eighth in time with the tempo; App → Echo still sets its level. A 22 Hz
  high-pass removes rumble.
- **Output** is the new last row of App → Volume & effects: Auto, Speaker or
  Headphones. Speaker adds octave harmonics of the bass, a little drum click, and
  cuts below about 70 Hz for the small speaker; Headphones plays full range. Auto
  follows the device's headphone jack and shows **Auto (speaker)** or
  **Auto (headphones)**. Where the jack is unknown, such as the desktop preview,
  it shows **Auto** and plays full range.
- **Voicing** is the new last row of the Chord and Pad tabs: Close (near middle C,
  moving to the nearest notes of the last chord), Open (spread wide), Power (root,
  fifth and octave) or Rich (adds 9ths and 7ths without the root). Chord starts on
  Close and Pad on Open. While the Bass layer plays, full Chord and Pad hits leave
  the root to the bass; power chords and a pattern's lone root keep it.
- Arp plays an octave above the chord. Steel-string and electric guitar, harp and
  plucked ethnic-string Chord sounds (programs 25–31, 46 and 104–107) strum: down on
  the beat, and up, quicker and softer, on offbeat eighths. Nylon stays fingerpicked. Backing notes vary a little in timing
  and velocity.
- **Front sound** has a new third row, **Level**: the front's level in this style,
  0–100% in 5% steps, like a layer mix.
- The built-in kits' kicks are short clicks (13–42 ms), so a short synthesized low
  thump now sounds under each kick.
- New patterns: Chord **Pop strum**, **Chords on 2 and 3** and **Rock eighths**;
  Drums **Chimes** (bell tree, ride bell, triangle and a soft shaker, with no kick
  or snare).
- Every built-in style was re-arranged for the built-in bank, with all five layers
  selected, a voicing for Chord and Pad, a pan position and reverb and echo sends
  for each part, and a reverb time. Drum kits are the built-in ones; the styles do
  not use the optional FluidR3 packs. Every style starts with swing Off.

  | Style | BPM / reverb | Chord | Arp | Pad | Bass | Drums | Front |
  |---|---|---|---|---|---|---|---|
  | Dream | 70 / 4.8 s | Slow Strings, Sustained, Close | Music Box, Up / down, with echo | Halo Pad, Slow swell, Open | Fretless, Held root | Standard, Chimes | Vibraphone, 2.6 s release |
  | Pop | 100 / 1.5 s | Steel-string Guitar, Pop strum, Close | Piano, Broken eighths | Slow Strings, Steady, Open | Fingered, Driving eighths | Standard, Pop | Piano |
  | R&B | 78 / 1.3 s | Electric Piano, Syncopated, Rich | Vibraphone, Syncopated notes | Warm Pad, Steady, Open | Fretless, Syncopated | Standard, R&B groove | Alto Sax |
  | Rock | 120 / 1.1 s | Overdriven Guitar, Rock eighths, Power | Clean Electric Guitar, Up / down | Organ 3, Steady, Open | Picked, Driving eighths | Standard, Rock | Distortion Guitar |
  | Waltz | 92 / 2.2 s | Piano, Chords on 2 and 3, Close | Harp, Broken 3/4 | Strings, Slow swell, Open | Acoustic, Waltz 3/4 | Brush, Waltz 3/4 | Oboe |
  | Bossa | 132 / 1.3 s | Nylon Guitar, Bossa accompaniment, Close | Vibraphone, Bossa mixed | Warm Pad, Steady, Open | Acoustic, Bossa nova | Jazz, Bossa nova | Flute |
  | Dance | 124 / 1.8 s | Synth Brass, Offbeat, Close | Saw Lead, Sixteenths up | Polysynth, Pulse, Open | Synth Bass, Offbeat | TR-808, Four on the floor | Square Lead |

  Unedited copies from 1.2, 1.3 and both earlier 1.4 development builds upgrade to
  these styles. Edited and your own presets keep their settings; presets from
  before 1.4 start Bass and Drums selected at 0%.
- Preset files add `swing_mode` (`off`, `8ths`, `16ths`), `swing_amount`,
  `chord_voicing` and `pad_voicing` (`close`, `open`, `power`, `rich`),
  `reverb_ms`, `front_mix`, and per-part `<part>_pan`, `<part>_reverb` and
  `<part>_echo` for front, chord, arp, pad, bass and drums. Pan and the sends
  are set only in the file. The earlier development key `swing` (50–75) is
  accepted and ignored.
- Presets: Harp, Keys and Organ are retired, and **Dance** joins. Seven styles:
  Dream, Pop, R&B, Rock, Waltz, Bossa, Dance. A session on a retired preset opens
  Dream. Waltz now plays in 3/4 throughout (its Arp had played a 4/4 line since 1.3).
- Drum patterns were rewritten so each has its own character: open hi-hats,
  ride, pedal hi-hat, ghost snares, claps and shaker, not only kick and snare.
  Backing accents now set note velocity once, instead of velocity and volume, so
  soft hi-hats and ghost notes stay audible.
- New Arp patterns **Sixteenths up** and **Two octaves**. **Broken 3/4** is listed
  for every preset.
- More scales. Chord config **Scale** offers Major, Natural minor, Harmonic minor,
  Melodic minor, Dorian, Mixolydian, Lydian and Phrygian. Front notes **Scale**
  also offers Major pentatonic, Minor pentatonic and Blues; with fewer notes than
  keys, Z-M continue into the next octave.
- 25 more named sounds from the built-in bank, in new **Winds** and **Synths**
  categories and the existing ones: Clavinet, Marimba, Accordion, Acoustic and
  Fingered Bass, Banjo, Kalimba, Violin, Cello, Pizzicato, Slow and Synth Strings,
  Choir, Flute, Clarinet, Alto Sax, Trumpet, Brass Section, Polysynth, Halo and
  Sweep Pad, Square and Saw Lead, Synth Bass and Synth Brass.
- Optional FluidR3 packs (MIT): `scripts/fetch_sound_packs.py` builds
  **fluid-synths** (six synth sounds, 1.0 MiB) and **fluid-drums** (seven kits,
  14.0 MiB) from the pinned Debian `fluid-soundfont-gm` package. Pack kits appear
  in Drums → Kit. Pack manifests accept the `Winds`, `Synths` and `Drums`
  categories; `Drums` sounds must use bank 128.
- New developer tool `zerochord-styles` renders each style playing I–vi–IV–V
  with a melody to WAV and prints each layer's level and frequency balance;
  `--levels` prints each sound's measured loudness and matching gain.
- Tab and the 1-N tab jump are hinted on the bottom line instead of a keycap
  beside the tabs.

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
