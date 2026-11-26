# Presets and customization

A preset remembers its sounds, envelopes, backing layers, patterns, and tempo.
For playing and menu controls, see the [user guide](USER_GUIDE.md).

- [Factory presets](#factory-starting-points)
- [Edit and save on the instrument](#on-the-instrument)
- [Edit preset files](#editable-toml-files)
- [Imported sounds](#imported-sounds)
- [App settings and mixing](#app-settings-and-mixing)

## Factory starting points

Dream is selected on a fresh launch. Press `9` / `0` to try the presets.

| Preset | Front | Chord / Arp | Pad | Initial backing | BPM |
| --- | --- | --- | --- | --- | --- |
| Dream | Music Box | Airy Pad | Warm Pad | Chord + Arp + Pad / Sustained + Up + Slow swell | 80 |
| Harp | Harp | Nylon Guitar | Strings | Pad / Steady | 96 |
| Keys | Piano | Piano | Warm Pad | Chord / Sustained | 100 |
| Organ | Organ | Organ | Warm Pad | Chord / Sustained | 100 |
| Pop | Steel-string Guitar | Electric Piano | Warm Pad | Arp / Broken eighths | 112 |
| R&B | Electric Piano | Electric Piano | Warm Pad | Chord / Syncopated | 82 |
| Rock | Overdriven Guitar | Clean Electric Guitar | Organ | Chord / Every eighth | 120 |
| Waltz | Celesta | Piano | Strings | Chord / Bass/chord 3/4 | 96 |
| Bossa | Vibraphone | Nylon Guitar | Warm Pad | Chord / Bossa accompaniment | 128 |

Dream and Harp use the **Sustained** chord pattern by default. Harp starts
with only the **Pad** layer enabled.

## On the instrument

Press `Tab` and open **Style**. The tab remembers the last row you selected.
See the [keyboard reference](USER_GUIDE.md#keyboard-reference) for navigation.

- **Preview a change.** `Z` / `C` (← / →) step a number, such as an envelope
  time, and audition it at once. For a sound, pattern, or key, open its list
  with `Enter`; moving to an option auditions it. Leaving the option stops the
  preview. Pattern previews run for up to
  eight seconds. Press `Space` to stop playback, `Enter` to apply the choice,
  or `Esc` to cancel. Layer mutes and mix in **Backing → Layers** apply at once.
- **Reset one setting.** Press `Backspace` on its row. In an open picker,
  `Backspace` highlights the default. Press `Enter` to apply it.
- **Make a copy.** Choose **Save as new preset** and press `Enter` to confirm. This writes a
  copy immediately as User001, User002, and so on. The copied settings become
  its defaults. Edit the TOML `name` to rename it.
- **Start over on a preset.** Choose **Restore preset** and confirm to reset
  the active preset's musical settings. App settings apply across presets.

An asterisk (`*`) marks a preset or setting that differs from its defaults.
Hold `Esc` to exit and save your edits. A forced stop or power loss can lose
edits made since you last saved.

![Style settings showing Chord, Arp and Pad patterns in order](images/release-style.png)

![App tab and app-wide controls](images/app-settings.png)

### Tempo and defaults

Press `P` or open **Style → Tempo**. Type a value from 40 to 240 BPM and
press `Enter` to apply it or `Esc` to cancel. `Backspace` deletes digits.
`R` selects the preset default for confirmation. ZeroChord remembers the
last preset you used. Stock defaults upgrade with the app, and customized
presets keep their values.

## Sound and envelope order

Style rows are Preset, Tempo, Backing, Front sound, Save as new preset, and
Restore preset. **Backing layers** has a Layers tab, then Chord, Arp, and Pad tabs with
Pattern (Pad: Motion), Sound, and Envelope rows. **Front sound** has Sound and
Envelope rows. Chord and Arp have separate sounds and envelopes.

Each envelope page has Attack, Decay, Sustain, Release, and Restore envelope
rows. ←/→ step a control through its values with a preview; holding repeats.
Enter opens an auditioning choice list; Enter applies the choice and Esc
cancels. Attack, Decay and Release use milliseconds (0–5000). Sustain is 0–100%.
The list includes the sound's own value near middle C, read from the SoundFont,
in numeric order. Choosing it keeps following the sound. **Default** marks the
preset's value. Chord and Arp bound slow attacks and long releases for clarity;
explicit envelope values override those bounds. A one-shot sample
still ends when its recorded audio runs out; Sustain cannot make it loop.
Backspace restores one control to the current preset's default. Restore envelope
resets only that part; Restore preset resets only the current preset.

Custom preset TOML accepts optional `front_attack_ms`, `front_decay_ms`,
`front_sustain_percent`, `front_release_ms`, with equivalent `chord_`, `arp_`, and
`pad_` keys. Omitted controls (or -1) use the sound's envelope. Echo is a separate
setting. `arp_sound` / `arp_sound_id` set the Arp sound, and `chord_mix`,
`arp_mix`, and `pad_mix` (0–100, default 100) set the layer mix. A file from
1.2 or earlier without Arp keys keeps using its Chord sound and envelope for Arp.

## Chord roots and front scale

**App → Chord config** starts each scope with **Octave**. The next rows are **Key**
and **Scale** on **All**, and **Root** on a degree tab. For **Root**, choose a note from C to B
or select **Follow Key**.
Default chord roots use the degrees of the major or natural minor Scale. Custom
roots remain fixed when Key changes. The default octave is 3. Each degree can use an absolute octave or
**Follow All**. These settings apply across presets.

`Backspace` restores the highlighted mapping at its current scope. The
**Restore defaults...** action resets the key or root, octave, and four chord
combinations for that scope after confirmation. On **All**, it also resets
Hold, recognition timing, and degree overrides. See the
[chord configuration reference](FRONT_NOTES.md#chord-config) for details.

App → Front notes → All shows **Key** and **Scale** immediately after Front mode
when Scale notes is selected. Both follow Chord config by default; choosing
either sets the front's own key and scale together, and Backspace follows the
chord again. Settings from 1.2 keep sounding the same: a front natural-minor
scale becomes its own front key. Front/Backing volume defaults are 100%/80%;
saved user volume values are retained on upgrade.

## Backing layers

**O** opens **Style → Backing layers** on its Layers tab; Esc returns to playing. ↑/↓
(F/X) selects a row in Chord, Arp, Pad order. Enter mutes or unmutes the layer,
←/→ (Z/C) changes its mix in 5% steps, and both apply immediately without
restarting the harmony. A layer at 0% is silent; raising a muted layer unmutes
it, and unmuting a 0% layer restores an audible level. Backspace restores the
layer's preset mix and mute state. With every layer muted or silent, **Front
only** selects harmony silently for Z–M/K/L. Hold still controls whether the
harmony remains selected. Built-in presets select all three layers; layers they
did not use in 1.2 start at 30%.

**Chord** uses its own sound and envelope. Manual hit strikes once per chord selection;
Sustained holds the chord without retriggering. Every beat, Every eighth,
Offbeat and Syncopated trigger simultaneous chord notes. Bass/chord 3/4 plays
low bass then two upper chord hits; Bossa accompaniment alternates low root/fifth
under syncopated upper chords. Bass events are voiced an octave lower.

**Arp** uses its own sound and envelope and has its own pattern clock.
Up, Down, Up/down, Root and upper notes and Broken eighths use flowing 95% gates.
Up/down repeats its endpoint notes at each turnaround to make a regular
four-beat phrase. Syncopated notes deliberately includes rests. Legacy mixed
Rock/Waltz/Bossa patterns retain their IDs and remain visible when selected in
an older saved setup. New presets use the dedicated Chord equivalents.
These are original accompaniment patterns, with no drums or Omnichord rhythm data.

**Pad** sustains its own sound and envelope. Steady, Slow swell (four 4/4 bars),
Breathe (two beats), and Pulse (one beat) affect only the Pad layer's volume.

Layers share harmony and tempo. Each layer plays at its mix, and several loud
layers are balanced by 1/√(sum of squared mixes) to leave mixing headroom; Front volume and timbre stay independent. Chord/Arp
preserve native sample decay/sustain instead of forcing every sound into the
same short envelope. Their attack is bounded to 10 ms and release to 200 ms
(250 ms for Sustained); explicit envelope settings override these bounds.

Chord changes retain the running phase. Tempo edits change the clock rate
without restarting it. Chord and Arp pattern changes each apply at their current
pattern boundary. Starting or stopping a layer, or a preset change, starts a
new phrase; a mix change alone keeps it.
Settings entry stops performance; auditions use a temporary setup. Space stops
all layers and queued front strums while preserving settings.

## Editable TOML files

The app uses `$XDG_CONFIG_HOME/zerochord/`, falling back to
`~/.config/zerochord/`. On the first normal launch it exports readable defaults:

```
zerochord/
  presets/factory/harp.toml     # Harp, Dream, Keys, Organ, Pop, R&B, Rock, Waltz, Bossa
  presets/user/user-001.toml    # Your saved/copied presets
  working/dream.toml           # Remembered edits; defaults remain separate
  session.toml                # Selected stable preset ID
  settings.conf               # App controls and legacy-compatible state
```

Close ZeroChord before editing files. It reloads them on next launch. Editing a
factory TOML changes that preset's defaults; existing working edits are retained
until you restore the preset/field. To add a preset, copy a complete factory file
into `presets/user/`, give it a unique lowercase `id` and a display `name`, and
restart. Save-as produces complete examples automatically. These files are data;
the app does not execute them.

Example fields from a complete preset:

```toml
schema_version = 2
id = 'my-pop'
name = 'My Pop'
mode = 'Arp'                    # Also: 'Chord + Pad', 'Chord + Arp', 'Pad + Arp', 'Chord + Pad + Arp', 'Front only'
bpm = 112
backing_sound = 4               # Chord; GM programs are ZERO-based: 0–127
arp_sound = 4                   # Arp; omitted means the Chord sound
pad_sound = 89                  # Independent Pad timbre
arp_mix = 100                   # Also chord_mix and pad_mix, 0–100
foreground_sound = 46
chord_pattern = 'quarter'
arp_pattern = 'pop'
pad_motion = 'steady'
```

Preset files keep their existing `mode` spellings for compatibility. The menus
show backing combinations in Chord, Arp, Pad order. Chord, Arp, and Pad sounds
have independent defaults and remembered edits. Older presets without `pad_sound`
start with their previous `backing_sound` for both, preserving their sound.

BPM ranges from 40 to 240. App values live in `settings.conf`, including volumes,
echo (0–60%), front modes/mappings and strum times (60–600 ms), key/scale, Hold,
and combinations with their recognition window (0–120 ms). Legacy
preset volume, level, echo, and sweep fields are accepted but ignored.
Version 1 preset files remain readable, but their former app fields no longer
change the keyboard or key when selecting/restoring a preset. Newly saved
version 2 presets contain only style fields.

Pattern IDs:

| Part | IDs |
| --- | --- |
| Arp | `up`, `down`, `up_down`, `alternating`, `pop`, `rnb`, `rock`, `waltz`, `bossa` |
| Chord | `manual`, `quarter`, `offbeat`, `syncopated`, `waltz`, `sustained`, `eighth`, `bossa` |
| Pad | `steady`, `swell`, `breathe`, `pulse` |

Presets select these built-in patterns; editing arbitrary rhythmic steps is
not implemented. Sparse user files use the base Harp defaults for omitted fields,
so copying a complete exported file is the clearest way to start. Working files
are complete snapshots; subsequent changes to factory files do not replace their
remembered settings automatically.

The loader rejects invalid IDs/ranges/types/pattern names, unknown fields,
unsupported versions, duplicate IDs, and files over 128 KiB; malformed presets
are skipped with a diagnostic. There is a limit of 128 presets. A bad factory
file falls back to its compiled defaults. `--no-save` reads existing settings but
creates no config files, including when testing save-as.

Existing style settings migrate into the active preset's working copy;
key/scale, Hold, and mappings remain app-wide in settings.conf. Subsequent launches
use stable preset IDs, so adding user files does not shift the selected preset.

## Imported sounds

The normal picker contains fourteen named sounds in category tabs. Keys includes
Piano, Electric Piano, Organ, Vibraphone, and Celesta. Plucked includes Harp,
Nylon Guitar, Steel-string Guitar, Clean Electric Guitar, Overdriven Guitar,
and Music Box. Strings contains Strings; Pads contains Warm Pad and Airy Pad.
All use the existing runtime SoundFont bank; no additional bank is bundled. Older program
numbers retain their sound; their embedded SoundFont name appears in a Saved
group when needed. Imported sounds can be used independently for each part.
Optional `pad_sound_id`, `backing_sound_id`, and `foreground_sound_id` strings
identify a sound as `pack-folder/sound-id`. Existing numeric sound fields retain
the built-in fallback. Missing packs keep the ID and play that fallback.
See [pack setup](../sounds/README.md) and [front mappings](FRONT_NOTES.md).

## App settings and mixing

**App** settings keep their values when selecting, saving, or restoring a preset,
and when upgrading ZeroChord. The **App** tab remembers the last selected row.

| Menu | Default |
| --- | --- |
| **Theme** | Dark |
| **Volume & effects** | System Vol uses the current system level. App Vol 100%, Backing 80%, Front 100%, Echo 15%, EQ 0 dB |
| **Chord config** | Hold on, key of C, 30 ms combination window, shared minichord mappings inherited by each degree |
| **Front notes** | Chord notes, with a 210 ms strum |

ZeroChord's output level is `App Vol × Backing or Front volume`. Combined backing
layers receive an additional 1/√(enabled layers) balance. App Vol scales both the
direct sound and echo. System Vol sets the device's playback level.

The app-wide three-band EQ shapes the entire Front, Chord, Arp and Pad mix including
echo, before App Vol and the output clamp. Each band ranges from −12 to
+12 dB in 1 dB steps, initially 0 dB (exact bypass). Low is a 250 Hz shelf,
Mid is a broad 1 kHz peak (Q ≈ 0.707), High is a 4 kHz shelf, using
[RBJ biquad formulas](https://www.w3.org/TR/audio-eq-cookbook/).
Use `Z` / `C` (← / →) on the band to adjust it and hear a sample. `Backspace`
resets it to 0 dB. EQ settings live in the app settings file.

## Factory sound details

The sound picker shows the source sample rate under the highlighted sound;
mixed-rate instruments show the range and “mixed”. These are rates from the
loaded SoundFont, including imported packs, not the 48 kHz output rate or a
quality score.

Acoustic and bell Front sounds retain native attack/decay/sustain; release is
180 ms, or 320 ms for Dream, Waltz and Bossa. Organ Front uses 5/100/85%/90
ADSR. Chord usually uses a 5 ms attack, native decay/sustain and 100 ms release;
Dream uses 8/400/35%/200, Organ 5/100/85%/100, Rock 3/160/45%/65.
Warm Pad uses 300/800/75%/550; Strings 150/600/75%/400; Organ Pad
80/300/75%/250. Times are milliseconds. These are editable musical starting
points; recordings that do not loop still end naturally.

Unmodified older factory defaults and working copies upgrade to these starting
points. Customized factory files and working copies keep their values. Restore
preset resets only the current preset to its loaded default.
