# ZeroChord 1.2.0 release record

Released locally on 2026-11-22. The installed candidate passed acceptance
checks on the development Cardputer. The previous release is `v1.1.0`.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `1.2.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | `dist/zerochord_1.2.0_arm64.deb` |
| Size | 723,192 bytes |
| SHA-256 | `0268cad6bf624b4a69e55486200923150ca1687385c70cb0d37aa89ba68b48ec` |
| Installed candidate SHA-256 | `f4a3486acac3eb04ca45f1214771e0e96fb198d4a02773d5893aba217746babb` |
| Checksum file | `dist/zerochord_1.2.0_arm64.deb.sha256` |
| Executable SHA-256 | `75f3df8f3715cb9efdd51d7bfb492850c9aae1a232a76dba9f972ab9acb3177b` |
| Maintainer configuration | Set `ZEROCHORD_MAINTAINER` locally before packaging |
| Dependencies | `libc6 (>= 2.36), libpulse0, timgm6mb-soundfont` |
| Source URL configuration | Optional `ZEROCHORD_HOMEPAGE`; see [.env.example](.env.example) |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The binary was cross-compiled on macOS with Zig for Linux ARM64 and glibc 2.36.
Debian supplies the runtime SoundFont as a dependency. The Grand Piano pack
is a separate download. The final package updates the bundled documentation.
Its executable and launcher files match the installed candidate byte for byte.

## User-facing deliverables

- [README](README.md): installation, first notes and essential controls.
- [User guide](docs/USER_GUIDE.md): modes, menus, presets, EQ, About and troubleshooting.
- [Release notes](CHANGELOG.md).
- [Original icon artwork](assets/zerochord-source.png) and [100×100 solid-background PNG](assets/share/images/zerochord.png).
- Five dark-theme store screenshots, reviewed at 320×170 for 1.2.0.
  [playing](store/screenshots/performance.png),
  [Style](store/screenshots/style.png),
  [Volume & effects](store/screenshots/volume-effects.png),
  [sounds](store/screenshots/sounds.png),
  [chord configuration](store/screenshots/chord-config.png).

The package also installs the guides and license notices under `/usr/share/doc/zerochord/`.

## Launcher installation

- Entry: `/usr/share/APPLaunch/applications/zerochord.desktop`
- Wrapper: `/usr/share/APPLaunch/bin/run-zerochord`
- Executable: `/usr/share/APPLaunch/bin/M5CardputerZero-ZeroChord`
- Icon: `/usr/share/APPLaunch/share/images/zerochord.png`

On the development device use `scripts/install_release_device.sh push|install|test`
(see [development notes](docs/DEVELOPMENT.md#releasing)). Manually, on the device:

```sh
sudo apt install ./zerochord_1.2.0_arm64.deb
systemctl --user restart APPLaunch.service
```

The app runs as the normal launcher user and keeps settings in `~/.config/zerochord/`.

## Validation

- [x] Six desktop suites, six ASan/UBSan suites, and 27 real-app flows passed.
- [x] Privacy scan found zero failures. Both warnings identify the public project link in About.
- [x] The device verified the candidate checksum. The final archive contains the
  same executable and launcher files.
- [x] `zerochord 1.2.0` installed on the Cardputer. `dpkg --verify` was clean and the launcher resumed.
- [x] The installed test passed evdev input, framebuffer readback, settings persistence,
  and clean exit. Digital capture recorded 420,000 audio frames with peak 9,954
  and zero queue overflows.
- [x] Store screenshots inspected at native resolution.
- [ ] About screen inspected on the device display (checked in the desktop render only).
- [ ] Optional Grand Piano pack and other hardware scripts exercised on the device.
- [ ] Physical keyboard feel and speaker/headphone listening accepted by the user.

Details and limits are in the [testing record](docs/TESTING.md).

## Store listing ready to use

Portal: <https://dev.cardputer.cc/#/upload>

- Application name: **ZeroChord**
- Summary: **A pocket chord-and-harp instrument for CardputerZero.**
- Category suggestion: **Media**
- Source repository: use the public URL configured in `ZEROCHORD_HOMEPAGE`.
- Email privacy: hide uploader email (`true`); choose a public package contact with `ZEROCHORD_MAINTAINER`.
- Icon: use the packaged PNG.
- Screenshots: the five files listed above, in that order.

Description:

> Make music with the CardputerZero keyboard. Select major, minor and seventh
> chords, combine buttons for richer harmonies, and pick or strum seven front
> notes. Play soft pads, clear sustained chords, rhythmic accompaniment or
> arpeggios. Combine Chord, Arp and Pad in Backing layers. Nine editable presets
> include separate Front, Chord/Arp and Pad sounds and envelopes. Customize
> chord roots, octaves, key combinations, and front-note layouts. Adjust the
> mix from Volume & effects with three-band EQ and Echo.
> Import named SoundFont packs from a TF card. Dark and light themes, printed
> arrow-key navigation, and per-setting defaults keep setup accessible.

## External publication

Not submitted. Package-name ownership, store category acceptance and uploader
sign-in are checked by the portal at submission. External submission requires
an affirmative user request; preparation and local installation do not submit it.
Record submission time, portal result and tracking URL here after submission.

## Git release

The local release is tagged `v1.2.0` on `main`. GitHub publication is pending.
