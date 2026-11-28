# ZeroChord 2.0.0 release record

Prepared on 2026-11-27. The installed candidate passed acceptance checks on the
development CardputerZero. The previous release is `v1.4.0`, published on GitHub
and in the CardputerZero Store.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `2.0.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | `dist/zerochord_2.0.0_arm64.deb` |
| Size | 828,912 bytes |
| SHA-256 | `45d000d26b452696a8e62cfd292123e72e4815c270fd151a3129713b33f60b2c` |
| Installed candidate SHA-256 | `162076e9e03689347a3987c961d1471b24b4fabe54155888659d143012ab37b0` |
| Checksum file | `dist/zerochord_2.0.0_arm64.deb.sha256` |
| Executable SHA-256 | `769af32732c78953bb34883eeff24e0188352cb48e3ea58655a65fb0f9f0313b` |
| Installed-Size | 2329 KiB |
| Maintainer configuration | Set `ZEROCHORD_MAINTAINER` locally before packaging |
| Dependencies | `libc6 (>= 2.36), libpulse0, pulseaudio-utils, timgm6mb-soundfont` |
| Source URL configuration | Optional `ZEROCHORD_HOMEPAGE`; see [.env.example](.env.example) |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The Debian VM cross-compiled the binary with Zig for Linux ARM64 and glibc 2.36.
Debian supplies the runtime SoundFont as a dependency. The Grand Piano, Synths
and Drums packs are separate downloads. The final package updates the bundled
testing record only: outside `usr/share/doc/zerochord/` its files and modes match
the installed candidate byte for byte, its executable is the tested one, and its
control fields differ only in `Installed-Size` (2328 to 2329). The installed test
ran on the candidate and was not repeated.

## User-facing deliverables

- [README](README.md) for the download, installation, and first notes, led by the looper.
- [User guide](docs/USER_GUIDE.md) for modes, menus, keyboard controls, and troubleshooting.
- [Release notes](CHANGELOG.md).
- [Original icon artwork](assets/zerochord-source.png) and [100×100 solid-background PNG](assets/share/images/zerochord.png).
- Five dark-theme store screenshots, reviewed at 320×170:
  [playing over a loop](store/screenshots/1-loop.png),
  [All front over a loop](store/screenshots/2-all-front.png),
  [backing layers](store/screenshots/3-backing-layers.png),
  [sounds](store/screenshots/4-sounds.png),
  [loop settings](store/screenshots/5-loop-settings.png).

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
sudo apt install ./zerochord_2.0.0_arm64.deb
systemctl --user restart APPLaunch.service
```

The app runs as the normal launcher user and keeps everything in `~/ZeroChord/`.
Its first launch moves an existing `~/.config/zerochord/` there.

## Validation

- [x] Eight desktop suites, the same eight with ASan/UBSan (LeakSanitizer on), and
  47 real-app flows passed, including the move to `~/ZeroChord/`.
- [x] Privacy scan found zero failures, for the tree and the archive. Warnings
  identify the intentional public project account, repository URL and noreply
  commit identity.
- [x] The device verified the archive checksum.
- [x] `zerochord 2.0.0` (installed candidate `162076e9…`) installed on the CardputerZero over 1.4.0. `dpkg --verify`
  was clean and the launcher resumed.
- [x] The installed test passed on the candidate: evdev input, framebuffer readback, settings
  persistence, a one-bar loop recorded through injected keys and saved, and clean
  exit. Digital capture recorded 520,800 stereo frames with peak 29,164 and zero
  queue overflows.
- [x] The development build's first launch moved the device's settings, presets,
  loop and three FluidR3 packs into `~/ZeroChord/`; a plain `.sf2` loaded and
  appeared under Custom.
- [x] Store screenshots reproduced byte for byte and inspected at native resolution.
- [ ] A real USB MIDI keyboard (notes, pedal, pitch bend, mod wheel) on the device.
- [ ] Hands-on playing of the looper, All front and 3/4 loops, and speaker/headphone
  listening, accepted by the user.

Details and limits are in the [testing record](docs/TESTING.md).

## CardputerZero Store

The `CHRD` description and screenshot order are in the [store listing](store/LISTING.md).

The 2.0.0 package (SHA-256 `45d000d2…`) was submitted on 2026-11-28 with
`czdev publish` in
[CardputerZero/packages #1032](https://github.com/CardputerZero/packages/pull/1032),
with the new listing and the five looper-first screenshots. Its commit, uploaded
package and manifest name only the project's GitHub account, its noreply address
and the project's public name; the uploaded package matches the final checksum.
Validation and capture checks passed, and it merged on 2026-11-28. The first two attempts stopped with a 404
while creating the branch, after GitHub had briefly suspended the account: the
fork had fallen 332 commits behind. Syncing the fork's `main` fixed it.

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

The release tag is `v2.0.0` on `main`, pushed on 2026-11-28 with `main`. The
GitHub Release for `v2.0.0` has the final package and checksum listed above, with
notes from the tagged changelog; its downloaded package matches the final SHA-256,
and its author and asset uploader are the project's account. `dev` stays local.
