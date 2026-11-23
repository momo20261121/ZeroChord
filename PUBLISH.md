# ZeroChord 1.1.0 release record

Status: **released.** Package built, installed and tested on the development Cardputer;
Git release: `main`, tagged `v1.1.0`. `dev` is local-only and is never pushed.
Store submission is separate from source-repository synchronization.
Previous release: `v1.0.0`.

## Package

| Field | Value |
| --- | --- |
| Package | `zerochord` |
| Version | `1.1.0`, from [VERSION.txt](VERSION.txt) |
| Architecture | `arm64` |
| Release file | `dist/zerochord_1.1.0_arm64.deb` (not tracked by Git) |
| Size | 811,624 bytes |
| SHA-256 | `6ac80aa183620c048fa03b969a0426b68aee21f95b71c26489d291377905f01a` |
| Checksum file | `dist/zerochord_1.1.0_arm64.deb.sha256` |
| Executable SHA-256 | `5339efb2391425b618559db24f093e5bf2f8ae552939da6b730c898aaa5aa095` |
| Maintainer configuration | Set `ZEROCHORD_MAINTAINER` locally before packaging |
| Dependencies | `libc6 (>= 2.36), libpulse0, timgm6mb-soundfont` |
| Source URL configuration | Optional `ZEROCHORD_HOMEPAGE`; see [.env.example](.env.example) |
| Source/icon license | MIT; [third-party notices](THIRD_PARTY_NOTICES.md) included |

The binary is cross-compiled on macOS (Zig, Linux ARM64, glibc 2.36) with a statically
linked C++ runtime. The runtime SoundFont is a Debian dependency, not included in the
archive; the optional Grand Piano pack is not packaged. No install hooks or credentials
are packaged. The installed executable on the device is byte-identical to the one in this
package; the final package differs from the one first installed only in bundled documentation.

## User-facing deliverables

- [README](README.md): installation, first notes and essential controls.
- [User guide](docs/USER_GUIDE.md): modes, menus, presets, EQ, About and troubleshooting.
- [Release notes](CHANGELOG.md).
- [Original icon artwork](assets/zerochord-source.png) and [100×100 solid-background PNG](assets/share/images/zerochord.png).
- Four 320×170 store screenshots, refreshed and inspected for 1.1.0:
  [playing](store/screenshots/performance.png),
  [Style](store/screenshots/style.png),
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
sudo apt install ./zerochord_1.1.0_arm64.deb
systemctl --user restart APPLaunch.service
```

The app runs as the normal launcher user and keeps settings in `~/.config/zerochord/`.

## Validation

- [x] Six desktop suites and six ASan/UBSan suites pass; 26 real-app flows pass.
- [x] Privacy scan of sources, commit metadata and package: no FAIL; one intentional WARN (public project URL in the About QR).
- [x] Package built from the tested executable; its checksum verified on the device.
- [x] `zerochord 1.1.0` installed on the Cardputer; `dpkg --verify` clean; launcher active.
- [x] `scripts/test_installed.py 1.1.0` passed: evdev input, framebuffer readback, 415,200 audio frames (peak 9,116, no queue overflows), persisted settings, clean exit.
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
- Screenshots: the four files listed above, in that order.

Description:

> Make music with the CardputerZero keyboard. Select major, minor and seventh
> chords, combine buttons for richer harmonies, and pick or strum seven front
> notes. Play soft pads, clear sustained chords, rhythmic accompaniment or
> arpeggios. Layer Chord, Pad and Arp backing with a checkbox popup. Nine editable presets include independent Front, Chord and Pad
> sounds, envelopes, tempo and patterns. Customize chord roots, octaves, key
> combinations and front-note layouts; shape the full mix with three-band EQ.
> Import named SoundFont packs from a TF card. Dark and light themes, printed
> arrow-key navigation, and per-setting defaults keep setup accessible.

## External publication

Not submitted. Package-name ownership, store category acceptance and uploader
sign-in are checked by the portal at submission. External submission requires
an affirmative user request; preparation and local installation do not submit it.
Record submission time, portal result and tracking URL here after submission.

## Git release

`dev` was merged into `main` with a merge commit and tagged `v1.1.0`; only `main` and the
tag are pushed to the public repository.
