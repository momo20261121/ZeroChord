# Presets and customization

A preset is the complete musical setup: key/scale, Pad/Chord/Arp mode, independent
backing and foreground sounds, all three pattern selections, BPM, Hold, part
volumes, preset level, echo, K/L sweep time, and shared/per-degree chord mappings.
The selected chord and currently pressed keys are performance state, not saved.
Startup is silent.

Only **Master volume** and **Theme** are app-wide. Preset switching and preset
restore preserve them. Output level is:

```
master volume × preset level × backing/front volume
```

At preset level 100%, the preset follows the master directly. A preset can have
a different default level, and the player can customize it. Master volume scales
both the direct sound and echo. It always controls the final output.

## On the instrument

- **9 / 0:** previous / next preset, including its BPM and remembered edits.
- **Tab:** one flat settings list; Up/Down navigates, Left/Right adjusts.
- **Enter:** open a choice list; Enter commits and Esc cancels. Sound, pattern,
  mode, key, and mapping choices audition without committing. Pattern auditions
  run for up to eight seconds; Space stops them.
- **Backspace:** restore the highlighted musical setting to the selected preset's
  default. In an open choice list, Backspace highlights the default; Enter still
  confirms it. Master volume and Theme restore their app defaults (100%, Dark).
- **Save as new preset:** confirm to create User001, User002, etc. Its current
  musical settings become its defaults. Rename it by editing its TOML `name`.
- **Restore preset:** confirmation resets every musical setting in the active
  preset. Other presets and app-wide controls are preserved.

A `*` by the preset name means its musical settings differ from its defaults.
A `*` beside a setting marks an edited field; choice lists label the default.
Master volume/Theme markers compare against app defaults. Changes are remembered
per preset and written on clean exit. Save as new preset also writes immediately.
A forced kill/power loss can lose edits since the last clean save.

Chord mappings retain **1 = All, 2–8 = II–VII, I**. Backspace restores just the
highlighted mapping at that scope to the preset default. Follow All removes a
per-degree override. The mapping-page restore action resets all four mappings
for that scope, with confirmation. A preset may define its own mapping defaults.

![Flat settings and app-wide controls](images/light-settings.png)

## The three backing modes

**Pad** sustains the backing sound. Motion choices are Steady, Slow swell (four
bars), Breathe (two beats), and Pulse (one beat). Motion changes backing volume;
foreground Z–M and K/L retain their own sound and articulation.

**Chord** plays all four chord notes simultaneously with a quick attack and
falling envelope. Manual hit plays once per chord-button press. Every beat,
Offbeat, Syncopated, and Waltz add repeating accompaniment. Hold retains the
harmony; it does not repeat Manual hit.

**Arp** plays rhythmic chord-tone patterns. Choices are Up, Down, Up/down,
Alternating bass, Pop, R&B, Rock, Waltz, and Bossa. Some style patterns include
upper-note groups alongside individual bass notes. These are original simple
accompaniments inspired by the style concept, without sampled drums or borrowed
Omnichord rhythm data. Waltz has three beats per bar; the other patterns use four.

**O** cycles Pad → Chord → Arp and immediately applies the selected mode to the
current chord. **Z–M** always play foreground notes; **K/L** always sweep those
seven notes, with the configured sweep duration. Backing Chord hits do not use
that sweep duration. **Space** stops playback and preserves all musical settings.

Chord changes retain the running rhythmic phase. Tap tempo changes the clock's
rate without restarting it. Pattern changes during playback take effect at the
next boundary of the current pattern; mode/preset changes start a new phrase.
Settings entry stops performance; its auditions use a separate temporary setup.

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
schema_version = 1
id = 'my-pop'
name = 'My Pop'
mode = 'Arp'                    # Pad, Chord, or Arp
bpm = 112
backing_sound = 4               # GM programs are ZERO-based: 0–127
foreground_sound = 46
backing_volume = 65             # 0–100
foreground_volume = 85
level = 100                    # Relative preset level, not master volume
arp_pattern = 'pop'
chord_pattern = 'quarter'
pad_motion = 'steady'
tonic = 0                      # C
```

Tonic values range from `0` (C) through `11` (B); `scale = 0` is major and `1` natural minor.
Other fields: `hold = 0/1`, `echo = 0..60`, `sweep_ms = 60..600`, BPM `40..240`.
`combinations` contains four quality names in top+middle, top+bottom,
middle+bottom, all-three order. `degree_combinations` contains seven such arrays
in **II, III, IV, V, VI, VII, I** order; use `'inherit'` to follow All.
Allowed qualities: `maj`, `min`, `7`, `maj7`, `m7`, `dim`, `aug`, `no3`, `sus2`, `sus4`.

Pattern IDs:

| Part | IDs |
| --- | --- |
| Arp | `up`, `down`, `up_down`, `alternating`, `pop`, `rnb`, `rock`, `waltz`, `bossa` |
| Chord | `manual`, `quarter`, `offbeat`, `syncopated`, `waltz` |
| Pad | `steady`, `swell`, `breathe`, `pulse` |

Version 1 selects these built-in patterns; editing arbitrary rhythmic steps is
not implemented. Sparse user files use the base Harp defaults for omitted fields,
so copying a complete exported file is the clearest way to start. Working files
are complete snapshots; subsequent changes to factory files do not replace their
remembered settings automatically.

The loader rejects invalid IDs/ranges/types/pattern names, unknown fields,
unsupported versions, duplicate IDs, and files over 128 KiB; malformed presets
are skipped with a diagnostic. There is a limit of 128 presets. A bad factory
file falls back to its compiled defaults. `--no-save` reads existing settings but
creates no config files, including when testing save-as.

Existing legacy settings are migrated once into the active preset's working
copy, preserving its key, tempo, volumes, and custom mappings. Subsequent launches
use stable preset IDs, so adding user files does not shift the selected preset.
