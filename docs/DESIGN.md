# ZeroChord design

Updated: 2026-11-21. This is a dated design record from the 0.1.0 prototype; it is not
kept in step with later releases. For current behavior see the [user guide](USER_GUIDE.md)
and [release notes](../CHANGELOG.md).

This is a living design record. The 0.1.0 prototype implements the agreed
controls and the working defaults described below. See the final section for
the concrete first-version behavior and remaining hands-on decisions.

## Intent

Make a small, immediate chord-and-harp instrument for CardputerZero. Preserve
minichord's relationship between chord selection and harmonically related harp
notes, while adapting the controls to the physical keyboard. Prioritize the
experience of playing over the number of features.

Playing model: play it like minichord. Physical posture is not a separate product
choice to resolve; adapt the controls to CardputerZero and validate them on device.

Sound direction: use openly licensed sound sources. Minichord and Omnichord guide
the musical interaction; an exact reproduction of their timbres is not required.
CardputerZero Piano is a reference for sound-bank playback and platform support.
See [audio sourcing and reuse research](AUDIO_AND_REUSE.md).

## Agreed chord layout

The seven columns follow II, III, IV, V, VI, VII, I. Key 8 is the tonic column,
not a separate upper-octave tonic chord. The selected key and scale determine
the root pitch classes. The three rows independently determine chord quality.

In C major:

| Row | II: D | III: E | IV: F | V: G | VI: A | VII: B | I: C |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Major | 2: D | 3: E | 4: F | 5: G | 6: A | 7: B | 8: C |
| Minor | Q: Dm | W: Em | E: Fm | R: Gm | T: Am | Y: Bm | U: Cm |
| Dominant seventh | A: D7 | S: E7 | D: F7 | F: G7 | G: A7 | H: B7 | J: C7 |

Letter names refer to physical keys and do not require Shift. The scale supplies
roots, not automatic diatonic chord qualities: D major, F minor, and C7 remain
available in C major. Dominant seventh is distinct from major seventh.

Example: 8, 5, T, 4 plays I, V, vi, IV. In C major this is C, G, Am, F;
in G major it is G, D, Em, C.

## Agreed minichord combination logic

Within one root column, use minichord's chord-quality combinations:

| Pressed rows | Result | C-major setting, first column |
| --- | --- | --- |
| Major + seventh | Major seventh | 2 + A: Dmaj7 |
| Minor + seventh | Minor seventh | Q + A: Dm7 |
| Major + minor | Diminished triad | 2 + Q: Ddim |
| All three | Augmented triad | 2 + Q + A: Daug |

Follow the reference's press-driven harmony changes: releasing part of a
combination must not select an unintended simpler chord. This does not imply
that sound sustains indefinitely with chord hold disabled; note release and
harmonic selection are separate behaviors.

Minichord also supports cross-column root borrowing, often producing slash
chords. Preserve this as the reference direction, but define base-chord
precedence, bass voicing, and interaction with hold before implementing it.
Do not silently interpret different columns as stacked independent chords.

## Control map

The chord grid, minichord combinations, preset/strum swap, and directional
consistency are confirmed. Other controls below retain the previously proposed
mapping as a working baseline; exact musical behavior remains open.

| Key | Function |
| --- | --- |
| 2–8 / Q–U / A–J | Chord grid above |
| 1 held with a chord selection | Raise the selected root by one semitone |
| Z X C V B N M | Seven ascending harp notes from the current chord |
| K | Descending strum: high to low, equivalent direction to M toward Z |
| L | Ascending strum: low to high, equivalent direction to Z toward M |
| 9 | Previous preset |
| 0 | Next preset |
| I | Toggle chord hold |
| O | Backing layers: mute and mix Chord, Arp, Pad, Bass and Drums. None audible is Front only |
| P | Tempo page: Tempo (typed, 40–240 BPM), Swing (Off / 8ths / 16ths) and Swing amount (0–100%) |
| Space | Tap: stop all sound. Hold: switch All front, where every key plays the front |
| Tab | Open/close settings |
| Esc | Back/close the current panel; long press returns home |

Direction convention: left means lower/previous; right means higher/next.
K is down, L is up. Up and down describe pitch movement, not a guitar-hand gesture.

The sharp modifier changes the root by a semitone; it is not a global change of
the configured key. Specify whether modifier changes alone retrigger anything
before implementation. Preserve normal system Fn shortcuts. In performance
mode F/Z/X/C are musical keys; in settings they may serve as navigation keys.

## Proposed interface

One main performance screen at 320 x 170, with the current chord prominent,
degree/root mapping, selected key and scale, hold/playback mode, and harp
activity. Prefer useful playing feedback over decorative animation.

Tab opens settings for key, scale, sound, chord/harp balance, effects, and
arpeggiator tempo/pattern. Keep frequent performance controls directly accessible.

## Working defaults for the first musical prototype

These are implementation starting points to validate through playing, rather
than additional product questions that must block progress.

1. **Chord hold and note lifetime.** Recommended starting point: selecting a chord
   sounds it immediately; hold defaults on so both hands can move to the harp.
   Existing harp notes keep their pitch and decay naturally when the chord changes;
   new presses follow the new chord. Space stops voices and the arpeggiator and
   clears the active chord latch without changing sound or key settings.
2. **Harp and strum behavior.** Define the seven pitches, octave/register, whether
   seventh chords change their spacing, and whether K/L perform one sweep per
   press or repeat while held. Recommended starting point: one sweep per press,
   with adjustable sweep time. K/L should traverse the same seven pitches as
   Z–M, in reverse/forward order respectively.
3. **Sounds.** Audition openly licensed sounds for a sustained chord part and a
   plucked harp part. Treat a preset as a pairing of both parts with their balance
   and effects. A warm sustained chord plus a bright plucked harp is a suggested
   first pairing; individual sound assets have not been chosen or bundled yet.
4. **Initial scale scope.** Major only for the first prototype is recommended.
   If natural minor or other scales are included, define root intervals and
   degree labels explicitly; the rows must continue to choose chord quality.

## Decisions that can follow the first prototype

- Exact chord voicings, bass range, inversions, and transitions between chords.
- Cross-column borrowing details and acceptable timing for rolled combinations.
- Arpeggiator patterns, beat-synchronized versus immediate chord changes, and
  whether the arpeggiator sounds alongside a sustained chord.
- Preset switching behavior, saved settings, startup state, and effect controls.
- Visual identity, icon, language support, implementation backend, and license.
- Whether drums, recording, looping, or MIDI belong in a later release.

## Prototype validation

Before treating the layout as final, test actual keyboard press/release events,
three-key combinations, sharp plus combinations, cross-column combinations, and
harp playing while chord keys are held. Check ghosting, missed releases, key
repeat, modifier processing, and collisions with system shortcuts.

Measure responsiveness from input to sound on the device and check that display
updates do not disrupt audio. Judge musical feel through headphones as well as
the built-in speaker. CPU and audio specifications establish plausibility, not
measured performance.

Current milestone: validate the playable prototype on the actual device.

## Implemented 0.1.0 behavior

- Major and natural minor both select roots. Chord rows remain explicit quality
  choices. Settings choose any of the 12 tonics.
- Hold defaults on. With hold off, releasing every chord key stops the chord and
  arp generation; existing one-shot harp notes finish their decay. Releasing
  part of a chord combination never changes the selected harmony.
- The first held column supplies the base chord. A newly pressed other column
  supplies slash bass, voiced below the root. If several other columns are held,
  the most recently pressed one wins. Releasing a bass key does not change the
  selected harmony until the next chord press. Rows in the borrowed column do
  not select another independent chord.
- Sharp is sampled on chord presses. Pressing/releasing 1 alone does not
  retrigger or transpose an existing chord.
- Chord notes use root/third/fifth/octave for triads and root/third/fifth/seventh
  for seventh chords. Diminished and augmented use their respective thirds and
  fifths. Harp notes start one octave above the chord root, repeating three-note
  triads or four-note sevenths to produce seven strictly ascending pitches.
  Slash bass changes the chord bass, while the harp follows the base harmony.
- Harp voices have independent trigger handles and retain pitch across chord
  changes. A Z–M note sounds while held; release (no sooner than 60 ms after the
  start) begins its envelope release. Strum notes last 1.6 seconds.
  K/L perform a single sweep; a new sweep replaces unfinished sweep events.
  A new chord cancels the unfinished sweep so it cannot play stale harmony.
- Settings keys: Tab next tab, 1–8 jump, ↑/↓ rows, ←/→ step numbers at once
  (one shared stepping path, repeat while held), Enter opens pages, choice lists
  and confirmations (rows marked >), Esc backs out. Leaving an option (row, tab,
  list, page, or settings) stops its preview.
- O and Style → Backing layers open one page: a Layers tab (Enter mutes, ←/→ mix, both
  live), Chord / Arp / Pad / Bass tabs for pattern, sound, and envelope (Chord
  and Pad end with Voicing: Close, Open, Power, Rich), and a Drums tab for kit and
  pattern. No layer audible means Front only. Front sound has Sound, Envelope and
  Level (the front's mix in this style).
- The Tempo page has three rows: Tempo (digits, Enter applies), Swing (Off /
  8ths / 16ths; Enter or ←/→) and Swing amount (←/→, 5% steps, dimmed while Off). Swing
  rows apply live; Esc keeps them and drops an unapplied tempo.
- App → Volume & effects ends with Output (Auto / Speaker / Headphones, a list).
  Auto follows the headphone jack where the device reports it.
  All five layers can run together. Front notes remain independent.
- Chord/Arp/Bass/Drums have separate pattern state on a common sixteenth-note
  clock; 8ths swing delays the "and", 16ths swing every second 16th. Bass follows the chord's lowest note
  (root or slash bass) in a low register. Drums play General MIDI kits (bank 128)
  and ignore the chord's notes.
  Pattern edits take effect at their current pattern boundary; harmony changes
  retain phase. Starting/stopping layers or presets starts a new phrase; mix
  changes update ringing notes.
- Chord patterns include Manual hit, Sustained, Every beat, Every eighth,
  Offbeat, Syncopated, Bass/chord 3/4, Bossa accompaniment, Pop strum, Chords on
  2 and 3, and Rock eighths. Bass events use
  a lower octave. Arps use descriptive names; legacy saved IDs remain readable.
- Pad motion (Steady, Slow swell, Breathe, Pulse) modulates only Pad gain.
  Each layer mix is an independent fader over level-matched sounds; a limiter
  provides the headroom. Front gain is independent.
- Chord/Arp/Bass bound attack to 10 ms and release to 200 ms (250 ms for Sustained),
  preserve native decay/sustain, then apply explicit envelope overrides.
  Chord, Arp, Pad, and Bass each keep a separate sound and envelope. Layer changes release backing without
  silencing Front. Panic removes all synth voices and echo.
- Style presets include BPM, swing, enabled layers, patterns, and independent sounds. Each has
  editable TOML defaults plus a remembered working snapshot. Individual fields
  and entire presets can be restored; save-as creates a new user default.
  App-wide theme, master/backing/front volume, echo, sweep, key/scale, Hold and chord mappings survive
  preset switches/restores.
  See [preset behavior and format](PRESETS.md).
- Space clears voices, pending events and the active latch, preserving settings.
- Entering/leaving settings stops performance sound. Settings save on clean
  exit. Startup has no active chord and produces no sound.
- Combination customization preserves minichord defaults while adding no3, sus2,
  and sus4. Four shared combined-press mappings have independent per-degree
  overrides; single-button rows remain major/minor/7. Key/scale changes keep the
  mappings attached to their degrees. no3 uses root/fifth octaves, with a two-note
  harp cycle; suspended chords replace the third with a second/fourth.
- Tab → App → Chord config opens a dedicated editor. 1 selects All; 2–8 select
  II–VII then I. A visible scope strip and Shared/Inherited/Custom labels show the
  target and mapping source. ↑/↓ navigates; Enter opens a scrolling
  choice list, Enter commits, Esc cancels. Esc returns one level; Tab closes setup.
  F/X and Z/C remain alternative navigation keys. Number shortcuts are ignored
  while a picker is open to prevent changing its target accidentally.
- Sound and chord choices audition bounded chord/harp voices without mutating
  saved settings. Confirm applies the value; cancel, Tab, long Esc, and signal
  exit discard an unconfirmed choice. ←/→ switches Style/App tabs; settings change through Enter editors. Per-degree Follow All removes one override; confirmed degree
  reset restores its four app-default mappings. Confirmed shared reset restores the
  app-default mappings and overrides, preserving unrelated settings.
- The initial UI is English. Internationalization, recordings, drums, MIDI,
  smoother chord voice-leading, and custom rhythm-step editing are future work.

These choices make the first version concrete; they remain adjustable after
physical keyboard and listening tests.

## References

- [Minichord user manual](https://minichord.com/user_manual/)
- [Minichord source and licensing information](https://minichord.com/source/)
- [OM-108 owner's guide](https://www.omnichord-heaven.com/downloads/manuals/OM-108_Owners_Guide.html)
- [CardputerZero keyboard specification](https://cardputer.cc/#/documents/cp0-keys)
- [CardputerZero development guidelines](https://cardputer.cc/#/documents/cp0-dev)
- [CardputerZero hardware](https://docs.m5stack.com/en/CardputerZero)
- [Peripheral development guide](https://github.com/CardputerZero/Template/wiki/Peripheral-Development-Guide)
- [CardputerZero Piano](https://github.com/CardputerZero/Piano)
- [Minichord source repository](https://github.com/BenjaminPoilve/minichord)
