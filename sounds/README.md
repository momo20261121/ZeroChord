# Custom sounds

On CardputerZero, put SoundFonts in `~/ZeroChord/sounds/` on the TF card and
restart ZeroChord. Alternatively use `~/.config/zerochord/sounds/`, or set
`ZEROCHORD_SOUNDS_DIR` to another directory. Separately mounted cards are scanned
for `ZeroChord/sounds/` beneath `/media`, `/mnt`, or `/run/media`.

A plain `my-sounds.sf2` works directly: its embedded names appear in **Custom**.
Use lowercase letters, numbers, and hyphens for the file or pack folder name.
For a small, clearly named selection, create a folder:

```
sounds/
  my-pack/
    bank.sf2
    pack.toml
```

Example `pack.toml` (the bank must contain the specified programs):

```toml
file = 'bank.sf2'

[[sounds]]
id = 'velvet'
name = 'Velvet Pad'
category = 'Pads'
program = 89
bank = 0

[[sounds]]
id = 'glass'
name = 'Glass Harp'
category = 'Plucked'
program = 46
```

Programs are zero-based. Categories: `Keys`, `Plucked`, `Strings`, `Winds`,
`Pads`, `Synths`, `Custom`, or `Drums`. Category defaults to Custom; bank defaults
to 0. `Drums` is for drum kits: `Drums` sounds must use `bank = 128`, and only
they may use it; otherwise the pack is skipped. They appear only in **Backing
layers → Drums → Kit**, after the built-in kits, and not in the sound pickers.
Kits in a plain SF2 without a manifest are listed under Custom instead. Each sound
needs a unique lowercase `id`, a readable `name`, and an existing bank/program.
Changing a name or category preserves the sound selection; keep folder and sound
IDs stable. Presets store these IDs and a built-in fallback, not transient menu
positions. Missing or rejected packs play the fallback and retain the original
saved ID.

Limits: 16 MiB per SF2, 32 MiB total custom SF2 files, eight custom banks,
64 sounds per pack, 256 custom sounds total. A plain SF2 exposes its first 64
presets; a manifest can choose the favorites explicitly. Bad packs are skipped
with a startup warning. Packs load once at startup, before audio starts; restart
after adding/editing/removing files. WAV sample importing is not included.
Use sounds whose licenses permit your intended use; ZeroChord adds no rights
to imported recordings or SoundFonts.

## Optional grand piano comparison

Run `python scripts/fetch_piano_pack.py` from the app repository to fetch the
checksum-verified [FluidR3 piano pack](fluid-grand/SOURCES.md). Copy the resulting
`fluid-grand` directory into a sound pack location above. **Grand Piano (FluidR3)**
appears in Keys for Front, Chord and Pad, alongside the original Piano. Its source
is 32 kHz; its name identifies the bank rather than claiming Hi-fi quality.
License and provenance travel with the pack. This optional download is not
bundled into the installed package.

## Optional FluidR3 synths and drum kits

Run `python scripts/fetch_sound_packs.py` from the app repository. It downloads
the pinned Debian `fluid-soundfont-gm` package into `.tools/`, checks every file
against a SHA-256, and cuts two MIT-licensed packs from FluidR3 GM without
processing the samples. Copy either folder into a sound pack location above.

- [`fluid-synths`](fluid-synths/SOURCES.md), 1.0 MiB: Square Lead, Saw Lead,
  Synth Bass, Synth Brass and Polysynth in Synths, and Synth Strings in Strings.
- [`fluid-drums`](fluid-drums/SOURCES.md), 14.0 MiB: Standard, Room, Power,
  Electronic, TR-808, Jazz and Brush kits for **Drums → Kit**. The Orchestra kit
  is left out to stay within the 16 MiB per-pack limit; the built-in Orchestra
  kit remains available. The built-in styles use the built-in kits; choose a
  pack kit in **Drums → Kit** to use it in a style.

Each sound's name ends in (FluidR3). Together with `fluid-grand`, the three packs
total about 22.5 MiB of the 32 MiB allowance. They are not bundled into the
installed package.
