# ZeroChord 1.4.0 release record

Prepared on 2026-11-26. The installed candidate passed acceptance checks on
the development CardputerZero. The previous release is `v1.3.0`, which was
tagged locally; the newest release on GitHub is `v1.2.1`.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `1.4.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | `dist/zerochord_1.4.0_arm64.deb` |
| Size | 749,856 bytes |
| SHA-256 | `c3c26957467d58dc5fb27501a955ab2d63058c663d85dbc45ab6201845ebca74` |
| Installed candidate SHA-256 | `8409e8632d9c1f7e5c4ceb5ed45075bcb777021475568efbdeabe80d654643b7` |
| Checksum file | `dist/zerochord_1.4.0_arm64.deb.sha256` |
| Executable SHA-256 | `b67aa6afe5c48964d1d48e37be27b1552f25ebbe24f5e2767fe4939658f4011c` |
| Maintainer configuration | Set `ZEROCHORD_MAINTAINER` locally before packaging |
| Dependencies | `libc6 (>= 2.36), libpulse0, pulseaudio-utils, timgm6mb-soundfont` |
| Source URL configuration | Optional `ZEROCHORD_HOMEPAGE`; see [.env.example](.env.example) |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The Debian VM cross-compiled the binary with Zig for Linux ARM64 and glibc 2.36.
Debian supplies the runtime SoundFont as a dependency. The Grand Piano, Synths
and Drums packs are separate downloads. The final package updates the bundled
changelog and testing record only: outside `usr/share/doc/zerochord/` its files
and modes match the installed candidate byte for byte, and its control fields
differ only in `Installed-Size` (2151 to 2152).

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
sudo apt install ./zerochord_1.4.0_arm64.deb
systemctl --user restart APPLaunch.service
```

The app runs as the normal launcher user and keeps settings in `~/.config/zerochord/`.

## Validation

- [x] Six desktop suites, six ASan/UBSan suites, 43 real-app flows and the
  playing-screen check passed. The desktop app built with ASan/UBSan passed the
  same flows, and 13 playing-screen states matched the full redraw pixel for pixel.
- [x] Privacy scan found zero failures, for the tree and both archives. Warnings
  identify the intentional public project account, repository URL and noreply
  commit identity.
- [x] The device verified the candidate checksum. The final archive contains the
  same executable and launcher files.
- [x] `zerochord 1.4.0` (installed candidate `8409e863…`) installed on the
  CardputerZero over 1.3.0. `dpkg --verify` was clean and the launcher resumed.
- [x] The installed test passed on the candidate: evdev input, framebuffer readback,
  settings persistence including Bass and Drums, and clean exit. Digital capture
  recorded 424,800 stereo frames with peak 29,164 and zero queue overflows.
- [x] Scripted latency measurement on the device: main loop 0.5 ms on average
  while playing, audio callbacks every 2.7 ms, no PipeWire xruns.
- [x] Store screenshots reproduced byte for byte and inspected at native resolution.
- [ ] Optional Synths, Drums and Grand Piano packs exercised on the device.
  `scripts/device_input_test.py` still walks the 1.2 menus.
- [ ] Physical keyboard feel (latency, held notes, repeating arrows), Auto output
  following the headphone jack, and speaker/headphone listening, accepted by the user.

Details and limits are in the [testing record](docs/TESTING.md).

## CardputerZero Store

The `CHRD` description and screenshot order are in the [store listing](store/LISTING.md).
The 1.4.0 package (SHA-256 `c3c26957…`) was submitted on 2026-11-26 with
`czdev publish` in
[CardputerZero/packages #888](https://github.com/CardputerZero/packages/pull/888).
Its commit, uploaded package and manifest name only the project's GitHub
account and noreply address; the uploaded package matches the final checksum.

PR #888 passed validation and merged on 2026-11-26. A listing-only update
(`czdev publish --meta-only`) then replaced the description with the current
[store listing](store/LISTING.md) text, which names the Bass and Drums layers
and swing; it passed validation and merged the same day in
[CardputerZero/packages #889](https://github.com/CardputerZero/packages/pull/889).

The 1.3.0 release was not submitted to the store.

The 1.2.1 submission passed validation and merged on 2026-11-24 in
[CardputerZero/packages #782](https://github.com/CardputerZero/packages/pull/782).

The 1.2.0 submission is recorded in [CardputerZero/packages #712](https://github.com/CardputerZero/packages/pull/712).

## Git release

The release tag is `v1.4.0` on `main`, pushed on 2026-11-26 with `main`. The
GitHub Release for `v1.4.0` has the final package and checksum listed above, with notes from the tagged
changelog; its downloaded package matches the final SHA-256. The `v1.3.0` tag
remains local.
