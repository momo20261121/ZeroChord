# ZeroChord

<img src="assets/share/images/zerochord.png" alt="ZeroChord icon" width="96" height="96">

**A pocket chord-and-harp instrument for CardputerZero. Version 1.2.0.**

Inspired by [**minichord**](https://minichord.com/) and the
[**Suzuki Omnichord**](https://www.suzuki-music.co.jp/special/omnichord_om-108/en/),
ZeroChord lets you play a chord with one hand and pick or strum its notes with the other.
Combine a clear Chord, a rhythmic Arp, and a soft Pad, or play Front only. Nine editable
presets give you a starting point; sounds, envelopes and tempo are adjustable.

![ZeroChord performance screen](docs/images/release-performance.png)

## Install

Copy `zerochord_1.2.0_arm64.deb` to the CardputerZero, then run there:

```sh
sudo apt install ./zerochord_1.2.0_arm64.deb
systemctl --user restart APPLaunch.service
```

Open **ZeroChord** from the launcher. The package uses the system sound bank;
APT installs `libpulse0` and `timgm6mb-soundfont` if needed. Settings and presets
are kept in your home directory and survive updates. No network connection is
needed to play once installed.

## Make your first sound

1. Press **8** for a C major chord. The app starts silently until you play.
2. Play **Z X C V B N M** to pick its notes. **L** strums up; **K** strums down.
3. Try **8 → 5 → T → 4** for C → G → Am → F in the default key of C.
4. Press **O** to choose backing layers (Chord, Arp, Pad, or none for Front only). **I** toggles Hold.
5. Press **Space** to stop sound. Hold **Esc** to return to the launcher.

## Essential controls

| Key | Action |
| --- | --- |
| 2–8 / Q–U / A–J | Major / minor / seventh chord rows |
| Z–M | Seven front notes |
| K / L | Down/up strum, or octave down/up when configured |
| 9 / 0 | Previous/next preset |
| 1 + chord | Sharpen that chord root |
| I / O / P | Hold / backing layers / type a tempo |
| Tab | Settings |
| Z / C | Previous/next settings tab (printed ←/→) |
| F / X | Previous/next row or choice (printed ↑/↓) |
| Enter / Esc | Open or confirm / cancel or go back |
| Backspace | Reset a field or select its default in a picker |
| Space / hold Esc | Stop sound / exit to launcher |

**Hold** keeps a chord selected after release. **Style → Chord pattern →
Sustained** keeps its sound going too. **Manual hit** sounds once.

Open **App → Volume & effects** for Master, Backing and Front volume, Echo,
and EQ.

## Using ZeroChord

- [User guide](docs/USER_GUIDE.md): playing, modes, settings, EQ and troubleshooting.
  Open Tab → **About** on the device for the version and a QR code to this repository.
- [Presets](docs/PRESETS.md): sounds, envelopes, defaults and editable files.
- [Front notes](docs/FRONT_NOTES.md): chord/scale modes and per-key mappings.
- [Custom sound packs](sounds/README.md): importing your own SoundFonts.
- [Release notes](CHANGELOG.md).

## For developers

- [Build and release](docs/DEVELOPMENT.md) and [testing](docs/TESTING.md).
- [Design record](docs/DESIGN.md), [audio sourcing](docs/AUDIO_AND_REUSE.md) and the
  [scenario review](docs/SCENARIO_REVIEW.md): dated project notes, not user documentation.

## Acknowledgments

- The **minichord** firmware informed chord combinations and how
  a selected chord responds to button releases. [minichord repository](https://github.com/BenjaminPoilve/minichord)
- The M5Stack CardputerZero Community’s built-in **Piano** app was a reference
  for SoundFont playback and device audio. Its repository also supplies the
  separately downloaded Grand Piano sound pack. [CardputerZero Piano repository](https://github.com/CardputerZero/Piano)
- The official **CardputerZero Template** was a reference for the desktop
  simulator and device build. [CardputerZero Template repository](https://github.com/CardputerZero/Template)

ZeroChord's source and original icon are [MIT licensed](LICENSE).
The runtime sound bank has its own license and is supplied separately; see
[third-party notices](THIRD_PARTY_NOTICES.md).
Source code is included in this repository.
