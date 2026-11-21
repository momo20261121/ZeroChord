# ZeroChord

A pocket chord-and-harp instrument for CardputerZero, inspired primarily by
[minichord](https://minichord.com/user_manual/) and also by the
[Suzuki Omnichord OM-108](https://www.omnichord-heaven.com/downloads/manuals/OM-108_Owners_Guide.html).

Status: playable 0.1.0 prototype, with an SDL desktop preview and a native Linux
framebuffer/evdev build. Uses LVGL 9.5, TinySoundFont, and miniaudio.

![ZeroChord running on the CardputerZero framebuffer](docs/images/performance.png)

Choose a key and scale, select chords through a three-row keyboard grid, then
pick individual harp notes or trigger strums. Chord roots follow scale degrees;
the rows independently choose major, minor, or dominant seventh quality.

- [Complete presets, individual resets, and editable TOML](docs/PRESETS.md)
- [Controls and musical behavior](docs/DESIGN.md)
- [Testing and device installation](docs/TESTING.md)
- [Audio sourcing, reference projects, and reuse licenses](docs/AUDIO_AND_REUSE.md)
- [Packaging and publishing record](PUBLISH.md)

The original app source is [MIT licensed](LICENSE). The runtime TimGM6mb sound
bank is GPL-2-only and supplied separately by the device's Debian package.
See [third-party notices](THIRD_PARTY_NOTICES.md). This directory has its own Git
repository; no remote or external publication is configured.

## Play

| Chord row | II | III | IV | V | VI | VII | I |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Major | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| Minor | Q | W | E | R | T | Y | U |
| Dominant 7 | A | S | D | F | G | H | J |

Choose the key/scale with Tab. For I–V–vi–IV, press **8, 5, T, 4**.
Combine rows in one column for maj7, m7, diminished, or augmented chords.
Hold one column and press another for a slash bass.

**Z X C V B N M** play ascending harp notes. **K** strums down, **L** strums up.
**9/0** select the previous/next preset. **I** toggles hold; **O** cycles Pad → Chord → Arp;
**P** taps tempo. Hold **1** when choosing a chord to sharpen its root.
**Space** stops all sound. **Tab** opens settings; **Up/Down** select and
**Left/Right** change values. **Enter** opens a choice list; **Enter** confirms
and **Esc** cancels. **F/X** and **Z/C** also work. Short **Esc** goes back; holding
**Esc** for 800 ms exits. System Home/forced quit is handled by the launcher;
SIGINT and SIGTERM also cleanly stop audio.

The **O** modes change what a chord button (for example **8**) plays:

- **Pad:** a continuous background chord while Hold is on.
- **Chord:** all notes strike together with a clear attack and falling envelope.
  Manual hit plays once per press; optional rhythmic patterns repeat the chord.
- **Arp:** rhythmic chord tones repeat, with Up/Down/Up-down, alternating bass,
  Pop, R&B, Rock, Waltz, and Bossa choices. Up at 100 BPM starts a note every 300 ms.

**K/L** remain manual foreground sweeps in every mode. The backing sound stays
separate from the foreground sound. Space stops everything and preserves the mode.
Pad motion can be Steady, Slow swell, Breathe, or Pulse.

Nine complete factory presets: Harp, Dream, Keys, Organ, Pop, R&B, Rock, Waltz,
and Bossa. Each includes key/scale, sounds, BPM, mode/patterns, Hold, effects,
levels, sweep duration, and chord mappings. Switching recalls that preset's
remembered edits. Only **Master volume** and **Theme** are app-wide.

**Backspace restores the highlighted setting to its preset default.** A `*` marks
customization. **Save as new preset** creates an editable copy; **Restore preset**
resets all musical settings with confirmation. Defaults and remembered edits are
separate TOML files under `~/.config/zerochord/` (or `$XDG_CONFIG_HOME/zerochord/`).
The active legacy setup migrates automatically. Changes save on clean exit;
startup is silent. See the [preset guide](docs/PRESETS.md) for the file format,
volume inheritance, patterns, and restore behavior.

### Customize chord combinations

Open **Tab → Chord combinations → Enter**. The four combined presses retain
minichord defaults: top+middle = dim, top+bottom = maj7, middle+bottom = m7,
all three = aug. Each can be assigned any chord quality, including **no3**
(root/fifth), **sus2**, and **sus4**. Single-button major/minor/7 rows stay fixed.

- **1** edits the shared mapping for all degrees.
- **2–8** select **II, III, IV, V, VI, VII, I**, matching the playing keyboard.
- **Up/Down** selects a combination; **Enter** opens its choices. Sound/chord
  choices audition automatically; Enter saves the highlighted choice, Esc cancels.
- Individual degrees start **Inherited**. Choosing a quality makes it **Custom**;
  **Follow All** removes that override. Shared edits preserve custom overrides.
- **Left/Right** changes a mapping directly. Reset actions require Enter and a
  confirmation; restore the degree or shared mappings to the preset defaults.
  Backspace restores only the highlighted mapping.

Mappings follow degrees when key/scale changes, and affect chord, harp, and arp
notes. Releasing part of a combination preserves the selected harmony.

## Build locally

Requires Python 3.9+, Git, a C/C++17 compiler, and Make. Dependencies and CMake
stay inside this project:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install cmake==3.31.6
python3 scripts/bootstrap.py
.venv/bin/cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Release
.venv/bin/cmake --build build/desktop -j4
```

Copy the device's `/usr/share/sounds/sf2/TimGM6mb.sf2` into
`.tools/soundfonts/TimGM6mb.sf2` for local audio/tests, or pass another compatible
GM bank using `--soundfont PATH` / `ZEROCHORD_SOUNDFONT`. A substituted bank may
sound different and keeps its own license. The bank is not checked into Git.

```sh
build/desktop/M5CardputerZero-ZeroChord
.venv/bin/ctest --test-dir build/desktop --output-on-failure
build/desktop/zerochord-render .tools/soundfonts/TimGM6mb.sf2 build/demo.wav
```

For hardware, configure with `-DZEROCHORD_DESKTOP=OFF` and
`-DZEROCHORD_TEST_SOUNDFONT=/usr/share/sounds/sf2/TimGM6mb.sf2`.
See the [testing guide](docs/TESTING.md) for the device workflow and remaining
hands-on checks.
