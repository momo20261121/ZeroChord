# Building ZeroChord

Run commands from the project root. User controls are in the [user guide](USER_GUIDE.md).

## Branches and releases

`main` contains tested releases, identified by `v<version>` tags (starting with
`v1.0.0`). Create a short-lived `feature/<name>` or `fix/<name>` branch before
developing changes. Merge it into `main` when the next release is ready, then
delete the merged branch. There is no permanent `dev` branch. Preserve existing
history; pushing branches or tags requires explicit authorization.

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

### Build for the device on macOS

Copy [.env.example](../.env.example) to `.env` and set the SSH host, optional SSH
config, and device directory there. Load it before deploying:

```sh
set -a; . ./.env; set +a
.venv/bin/python scripts/build_device.py
sh scripts/deploy_device.sh
```

The first command installs checksum-verified Zig 0.15.2 inside `.tools/`, then
cross-compiles the Linux ARM64 framebuffer app into `build/cross-device/`.
Compiler caches also stay in `.tools/`; subsequent builds are incremental.
No VM, device sysroot, or system-wide install is required. The target uses glibc
2.36 compatibility and a statically linked C++ runtime. See the
[official Zig downloads](https://ziglang.org/download/) for the compiler source.

Deployment uses `ZEROCHORD_SSH_HOST`, optional `ZEROCHORD_SSH_CONFIG`, and
`ZEROCHORD_REMOTE_DIR` (default `ZeroChord`, relative to the remote user's home).
The existing device directory must contain the development wrapper and previous
binary. Deployment verifies the transferred binary's checksum, stops
the old preview cleanly so settings save, retains a `.previous` binary, and
reopens the app. A launch failure restores the previous binary. Nothing compiles
on the CardputerZero. This updates the development preview; packaging is separate.

See the [testing guide](TESTING.md) for the device workflow and remaining
hands-on checks.

### Private configuration and package metadata

Keep device addresses, account details and credentials in ignored local files
or environment variables. Only the placeholder [.env.example](../.env.example)
is tracked. Its Git identity variables use the project name and an empty email.
The packaging command requires `ZEROCHORD_MAINTAINER` to contain a public
`Name <email>` contact; `ZEROCHORD_HOMEPAGE` is optional. Those values are written
into the resulting public Debian package, so never supply secrets there.
Required third-party copyright and license notices remain intact.

## Named sounds and front notes

The sound picker offers ten named sounds in Keys, Plucked, Strings, and Pads.
←/→ changes category; ↑/↓ auditions sounds; Enter saves; Esc cancels.
Older saved sounds remain available without expanding the built-in selection.
Pad, Chord/Arp, and Front can each use built-in or imported sounds.

Put `.sf2` files or named packs in `~/ZeroChord/sounds/` on the TF card and restart.
See [custom sound packs](../sounds/README.md) for locations, naming, and limits.
In **App → Front notes**, choose Chord notes or Scale notes on the All tab,
configure K/L as Strum or Octave shift, and set each key’s note and actual octave.
←/→ navigates All and Z–M tabs. See [front-note controls](FRONT_NOTES.md).
