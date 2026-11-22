# ZeroChord

<img src="assets/share/images/zerochord.png" alt="ZeroChord icon" width="96" height="96">

**A pocket chord-and-harp instrument for CardputerZero. Version 1.0.0.**

Play a chord with one hand and pick or strum its notes with the other.
Choose a soft Pad, a clear Chord, a rhythmic Arp, or Front only. Nine editable
presets give you a starting point; sounds, envelopes and tempo are adjustable.

![ZeroChord performance screen](docs/images/release-performance.png)

## Install

Copy `zerochord_1.0.0_arm64.deb` to the CardputerZero, then run there:

```sh
sudo apt install ./zerochord_1.0.0_arm64.deb
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
4. Press **O** to cycle Pad → Chord → Arp → Front only. **I** toggles Hold.
5. Press **Space** to stop sound. Hold **Esc** to return to the launcher.

## Essential controls

| Key | Action |
| --- | --- |
| 2–8 / Q–U / A–J | Major / minor / seventh chord rows |
| Z–M | Seven front notes |
| K / L | Down/up strum, or octave down/up when configured |
| 9 / 0 | Previous/next preset |
| 1 + chord | Sharpen that chord root |
| I / O / P | Hold / backing mode / type a tempo |
| Tab | Settings |
| Z / C | Previous/next settings tab (printed ←/→) |
| F / X | Previous/next row or choice (printed ↑/↓) |
| Enter / Esc | Open or confirm / cancel or go back |
| Backspace | Restore the selected setting's default |
| Space / hold Esc | Stop sound / exit to launcher |

**Hold** keeps a chord selected after release. **Chord → Sustained** keeps its
sound going too; **Manual hit** sounds once. Pad and Front use their own sounds.

## Learn and customize

- [User guide](docs/USER_GUIDE.md): playing, modes, settings, EQ and troubleshooting.
- [Presets](docs/PRESETS.md): sounds, envelopes, defaults and editable files.
- [Front notes](docs/FRONT_NOTES.md): chord/scale modes and per-key mappings.
- [Custom sound packs](sounds/README.md): importing your own SoundFonts.
- [Release notes](CHANGELOG.md).
- [Build instructions](docs/DEVELOPMENT.md) and [test details](docs/TESTING.md).

Inspired by [minichord](https://minichord.com/) and Suzuki Omnichord.
ZeroChord's source and original icon are [MIT licensed](LICENSE).
The runtime sound bank has its own license and is supplied separately; see
[third-party notices](THIRD_PARTY_NOTICES.md).
Source: [ZeroChord contributors/ZeroChord](https://example.invalid/ZeroChord).
