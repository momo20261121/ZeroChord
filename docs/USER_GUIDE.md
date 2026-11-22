# ZeroChord 1.0 user guide

ZeroChord turns your CardputerZero keyboard into a chord-and-harp instrument.
Select a chord, then play its individual front notes. The backing and front
parts have separate sounds and envelopes.

## Start playing

Open ZeroChord from APPLaunch. Startup is silent. The initial preset is Dream,
with a soft Pad backing and a bright front sound in the key of C.

Press **8**, then play **Z X C V B N M**. Try **8 → 5 → T → 4** for C, G, Am, F.
Press **Space** to stop everything. Hold **Esc** for about a second to exit.
Use a clean exit to save your settings; removing power can lose recent edits.

## Chord buttons

Each vertical column chooses one degree of the key. In the default key of C:

| Row / degree | II (D) | III (E) | IV (F) | V (G) | VI (A) | VII (B) | I (C) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Major | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| Minor | Q | W | E | R | T | Y | U |
| Seventh | A | S | D | F | G | H | J |

Combine buttons **within a column** for more chord qualities:

| Combination | Default |
| --- | --- |
| Top + middle | Diminished |
| Top + bottom | Major seventh |
| Middle + bottom | Minor seventh |
| All three | Augmented |

For example **8 + U** selects C diminished. Press the buttons close together;
the default 30 ms combination window groups them into one attack. Change that
window, or assign no3/sus2/sus4, in **App → Chord config**.
Holding **1** while selecting a chord sharpens its root and the front chord
notes that follow it. Front Scale notes follow their chosen key/scale instead.
Hold one column and press another to choose a different bass note.

## Backing modes and Hold

Press **O** to cycle through these modes. The pattern is selected in Style.

| Mode | What the chord buttons do |
| --- | --- |
| Pad | Play a soft continuous chord, with optional volume motion |
| Chord | Strike the chord notes together using the Chord sound |
| Arp | Repeat chord tones in the selected rhythmic pattern |
| Front only | Select harmony silently, ready for Z–M and K/L |

In Chord mode, **Manual hit** plays once, **Sustained** keeps the sound going,
and Every beat/Offbeat/Syncopated/Waltz repeat the chord rhythmically.
Arp choices include Up, Down, Up/down, Alternating bass, Pop, R&B, Rock,
Waltz and Bossa. They do not add a drum track.

**I** toggles **Hold**. On keeps the selected harmony after releasing its buttons.
Off stops backing when the chord buttons are released. Hold does not repeat
a Manual hit. Space stops the backing, front notes and queued strums.

## Front notes and K/L

**Z–M** runs from lower to higher notes. The screen puts the note and octave
above its keyboard label. **App → Front notes** has two modes:

- **Chord notes:** front notes follow the selected chord and its quality.
- **Scale notes:** seven notes follow the selected key and major/natural minor
  scale; they can play without selecting a chord first.

On All, choose the mode and K/L action. **Strum** sweeps M→Z with K and Z→M
with L; Strum time is the time for the complete sweep. **Octave shift** moves
front notes down with K and up with L. Each mode remembers its own action.
Move to a key tab to change its note and absolute octave. See
[front customization](FRONT_NOTES.md) for details.

## Settings without guesswork

Press **Tab**. **Z/C** switches tabs and **F/X** moves the selection, matching
the printed ←/→ and ↑/↓ arrows. Dedicated arrow events work too.

Press **Enter** to open an editor. Choose a value with ↑/↓ and Enter to save it;
Esc cancels a preview. Sound choices also use ←/→ for categories. Tempo uses
a number popup: type 40–240 and press Enter. **P** opens the same popup while
playing. Short Esc goes back; hold Esc exits the app.

**Style** starts with Preset, Tempo, Mode and patterns, followed by Front,
Chord and Pad sounds/envelopes. **App** contains Theme, volumes, Echo, EQ,
Hold, Front notes and Chord config.

### Presets and defaults

**9/0** changes presets. Each preset remembers its edited sounds, envelopes,
tempo and patterns. **Backspace** restores one highlighted field to its default;
inside an editor it highlights the default and Enter still confirms.

A **\*** means a setting differs from its default. **Save as new preset** makes
an editable copy. **Restore preset** resets only the current preset, after
confirmation. Other presets and App settings are preserved.

### Volumes, EQ and Echo

App has Master, Backing and Front volume. Defaults are 100%, 80% and 100%.
Master controls the complete output, including Echo.

**EQ Low/Mid/High** boosts or cuts the complete mix by −12 to +12 dB per band.
Start at 0 dB; try Low +3 dB for more bass, and compare while playing. The small
speaker has limited low-frequency output. Strong boosts can distort; lower
Master volume or reduce the boost if that happens. Backspace resets one band
to 0 dB. EQ is shared across presets.

**Echo** adds a quiet delayed repeat, about 230 ms later. It is separate from
the sound's envelope. Set it to 0% for a dry sound.

### Envelopes

Front, Chord and Pad each have their own envelope. Arp uses Chord's envelope.

| Control | Meaning |
| --- | --- |
| Attack | Time to rise to full level |
| Decay | Time to fall to the held level |
| Sustain | Level while the note is active |
| Release | Fade after the note stops |

**Sound default** keeps the sound's original behavior or the mode's default
chord articulation. A recorded one-shot sound cannot continue beyond its sample
just by increasing Sustain. Restore envelope affects only that part.

### Chord configuration

**App → Chord config** starts with All. **1** selects All; **2–8** selects
II–VII/I. ←/→ also switches scope. All has Octave and Key first; a degree has
its own Octave and absolute Root. Follow All/Follow Key removes an override.
Root, octave and combination mappings remain the same when switching presets.
Front Scale notes have their own major/minor setting under Front notes → All.

## Add your own sounds

Copy named SoundFont packs into `ZeroChord/sounds/` on the TF card, or
`~/ZeroChord/sounds/`, then restart the app. They appear alongside the small
built-in sound set. Front, Chord and Pad can each choose an imported sound.
See [the sound pack guide](../sounds/README.md) for the manifest and size limits.
Use sound files whose license permits your intended use.

## Troubleshooting

| Symptom | Try |
| --- | --- |
| Silent on startup | Press 8; startup is deliberately silent |
| Chord button is silent | Check Front only mode, Backing/Master volume, and Hold |
| Chord fades while Hold is On | Choose Chord pattern → Sustained; Manual hit sounds once |
| Low notes seem quiet | Try EQ Low +3 dB; compare with headphones |
| Sound distorts | Reduce EQ boosts or Master volume |
| A combination makes two attacks | Increase Combination window slightly |
| An imported sound is missing | Check its manifest/limits and restart; see the sound pack guide |
| Recent edits disappeared | Exit by holding Esc so settings are saved |

The [preset guide](PRESETS.md) describes editable files and advanced behavior.
For development and verification, see [building](DEVELOPMENT.md) and
[testing](TESTING.md).
