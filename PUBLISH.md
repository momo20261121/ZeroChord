# ZeroChord 1.3.0 release record

Prepared on 2026-11-25. The installed candidate passed acceptance checks on
the development CardputerZero. The previous release is `v1.2.1`.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `1.3.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | `dist/zerochord_1.3.0_arm64.deb` |
| Size | 705,744 bytes |
| SHA-256 | `36ebdd62c4fd959501895f8fd107cd04f79a3b529fd8d074c47b417be73f5848` |
| Installed candidate SHA-256 | `bee95a3940cfc683f2bcdfaa807c7eea69170d1ab1f60c2935d8eca91ac15963` |
| Checksum file | `dist/zerochord_1.3.0_arm64.deb.sha256` |
| Executable SHA-256 | `6c4e24943f6d2b268d431068a4d279fd900f708310570fea0a2504328db5efa0` |
| Maintainer configuration | Set `ZEROCHORD_MAINTAINER` locally before packaging |
| Dependencies | `libc6 (>= 2.36), libpulse0, pulseaudio-utils, timgm6mb-soundfont` |
| Source URL configuration | Optional `ZEROCHORD_HOMEPAGE`; see [.env.example](.env.example) |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The Debian VM cross-compiled the binary with Zig for Linux ARM64 and glibc 2.36.
Debian supplies the runtime SoundFont as a dependency. The Grand Piano pack
is a separate download. The final package updates the bundled documentation.
Its executable and launcher files match the installed candidate byte for byte.

## User-facing deliverables

- [README](README.md) for the download, installation, and first notes.
- [User guide](docs/USER_GUIDE.md) for modes, menus, keyboard controls, and troubleshooting.
- [Release notes](CHANGELOG.md).
- [Original icon artwork](assets/zerochord-source.png) and [100×100 solid-background PNG](assets/share/images/zerochord.png).
- Five dark-theme store screenshots, reviewed at 320×170.
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

From the development VM, use `scripts/install_release_device.sh push|install|test`.
See the [development notes](docs/DEVELOPMENT.md#releasing). To install manually
on the device, copy over the package and run:

```sh
sudo apt install ./zerochord_1.3.0_arm64.deb
systemctl --user restart APPLaunch.service
```

The app runs as the normal launcher user and keeps settings in `~/.config/zerochord/`.

## Validation

- [x] Six desktop suites, six ASan/UBSan suites, and 37 real-app flows passed.
- [x] Privacy scan found zero failures. Warnings identify intentional public project references.
- [x] The device verified the candidate checksum. The final archive contains the
  same executable and launcher files.
- [x] `zerochord 1.3.0` installed on the CardputerZero over 1.2.1. `dpkg --verify` was clean and the launcher resumed.
- [x] The installed test passed evdev input, framebuffer readback, settings persistence,
  and clean exit. Digital capture recorded 424,800 stereo frames with peak 9,271
  and zero queue overflows.
- [x] Store screenshots and the new Backing layers screenshot inspected at native resolution.
- [ ] About screen inspected on the device display (checked in the desktop render only).
- [ ] Optional Grand Piano pack and other hardware scripts exercised on the device.
  `scripts/device_input_test.py` still walks the 1.2 menus.
- [ ] Physical keyboard feel (held notes, repeating arrows) and speaker/headphone
  listening, including the 30% level for newly added layers, accepted by the user.

Details and limits are in the [testing record](docs/TESTING.md).

## CardputerZero Store

The `CHRD` description and screenshot order are in the [store listing](store/LISTING.md).
The 1.2.1 submission passed validation and merged on 2026-11-24 in
[CardputerZero/packages #782](https://github.com/CardputerZero/packages/pull/782).

The 1.2.0 submission is recorded in [CardputerZero/packages #712](https://github.com/CardputerZero/packages/pull/712).

## Git release

The release tag is `v1.3.0` on `main`. The GitHub Release uses the final package
and checksum listed above, with notes from the tagged changelog.
