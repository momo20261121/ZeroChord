# ZeroChord 1.0.0 release record

Status: **release prepared, installed, and tested on the development Cardputer.**
Git release: `main`, tagged `v1.0.0`; the completed `dev` branch is retired.
No store submission or Git push has been performed.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `1.0.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | [dist/zerochord_1.0.0_arm64.deb](dist/zerochord_1.0.0_arm64.deb) |
| Size | 784,212 bytes |
| SHA-256 | `de2d9795097b85b4cd0be0a461a7597f64df14f767dc7648b32979e5e3f89d1a` |
| Checksum file | [dist/zerochord_1.0.0_arm64.deb.sha256](dist/zerochord_1.0.0_arm64.deb.sha256) |
| Maintainer | ZeroChord contributors, `maintainers@example.invalid` |
| Dependencies | `libc6 (>= 2.36), libpulse0, timgm6mb-soundfont` |
| Source | <https://example.invalid/ZeroChord> |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The binary is cross-compiled on macOS, stripped at link time, and about 1.8 MB.
The C++ runtime is linked statically. The runtime SoundFont is a Debian dependency,
not included in the archive. No install hooks or credentials are packaged.

## User-facing deliverables

- [README](README.md): installation, first notes and essential controls.
- [User guide](docs/USER_GUIDE.md): modes, menus, presets, EQ and troubleshooting.
- [Release notes](CHANGELOG.md).
- [Approved original icon artwork](assets/zerochord-source.png) and [256×256 PNG](assets/share/images/zerochord.png): a simple three-note chord on a staff.
- Four clean 320×170 screenshots, reviewed at native resolution:
  [playing](store/screenshots/performance.png),
  [Style](store/screenshots/style.png),
  [sounds](store/screenshots/sounds.png),
  [chord configuration](store/screenshots/chord-config.png).

The package also installs the guides and license notices under
`/usr/share/doc/zerochord/`.

## Launcher installation

- Entry: `/usr/share/APPLaunch/applications/zerochord.desktop`
- Wrapper: `/usr/share/APPLaunch/bin/run-zerochord`
- Executable: `/usr/share/APPLaunch/bin/M5CardputerZero-ZeroChord`
- Icon: `/usr/share/APPLaunch/share/images/zerochord.png`
- `Terminal=false`; the wrapper passes arguments through to the native GUI.

The verified archive and checksum have been copied to the device home directory,
and `zerochord 1.0.0` is installed. For a manual reinstall, run there:

```sh
sudo dpkg -i ${HOME}/zerochord_1.0.0_arm64.deb
systemctl --user restart APPLaunch.service
```

All dependencies are already present on the development Cardputer. For another
unit, use `sudo apt install ./zerochord_1.0.0_arm64.deb` to resolve dependencies.
The app runs as the normal launcher user and retains existing settings in
`~/.config/zerochord/`.

## Validation

- [x] All six local regression suites pass.
- [x] All fifteen real-app Z/C/F/X keyboard flows pass, including reload.
- [x] Four screenshots and the icon are visually checked.
- [x] Native Debian tools parse/extract the final archive and verify its checksum.
- [x] Packaged ARM64 executable reports `ZeroChord 1.0.0` on the Cardputer.
- [x] Native library resolution succeeds; no missing libraries.
- [x] Package contains launcher entry, executable permissions, PNG and guides.
- [x] Installed package and launcher registration verified; its icon is visible and selecting it launches the installed executable.
- [x] Installed wrapper tested with evdev, framebuffer, live audio and clean exit.
- [ ] Physical keyboard feel and speaker/headphone listening accepted by the user.

A focused installed-release test is available as
[scripts/test_installed.py](scripts/test_installed.py). It uses isolated settings,
preserves the physical input configuration, and restores APPLaunch afterward.
The installed check passed with 384,000 recorded audio frames, peak 9,960,
and no audio queue overflows. EQ, Sustained chord pattern, and tempo persisted
in isolated test settings. Independent kernel framebuffer readback matched the
app capture within RGB565 quantization. The release was then opened from its
actual launcher icon and left running with the user's normal settings.
The approved three-note chord icon is included in the current archive; its
installed checksum matches the source export, and its rendering and launch
action were checked in APPLaunch after refreshing the launcher.

## Store listing ready to use

Portal: <https://dev.cardputer.cc/#/upload>

- Application name: **ZeroChord**
- Summary: **A pocket chord-and-harp instrument for CardputerZero.**
- Category suggestion: **Media**
- Source repository: <https://example.invalid/ZeroChord>
- Email privacy: hide uploader email (`true`); package uses a GitHub noreply address.
- Icon: use the packaged PNG.
- Screenshots: the four files listed above, in that order.

Description:

> Make music with the CardputerZero keyboard. Select major, minor and seventh
> chords, combine buttons for richer harmonies, and pick or strum seven front
> notes. Play soft pads, clear sustained chords, rhythmic accompaniment or
> arpeggios. Nine editable presets include independent Front, Chord and Pad
> sounds, envelopes, tempo and patterns. Customize chord roots, octaves, key
> combinations and front-note layouts; shape the full mix with three-band EQ.
> Import named SoundFont packs from a TF card. Dark and light themes, printed
> arrow-key navigation, and per-setting defaults keep setup accessible.

## External publication

Not submitted. Package-name ownership, store category acceptance and uploader
sign-in are checked by the portal at submission. External submission requires
an affirmative user request; preparation and local installation do not submit it.
Record submission time, portal result and tracking URL here after submission.
