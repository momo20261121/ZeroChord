# Building ZeroChord

Run commands from the project root. User controls are in the [user guide](USER_GUIDE.md).

## Branches and releases

`main` contains tested releases, identified by `v<version>` tags (starting with
`v1.0.0`). Use `dev` for the next release (currently 1.4.0) and merge into `main`
when the release is tested and ready. Preserve existing
history. `dev` stays local and is never pushed (`scripts/install_git_guard.sh`
installs a pre-push hook enforcing this); pushing `main` or a release tag requires
explicit authorization.

The `/publish` workflow (`.claude/skills/publish/SKILL.md`) runs a release: version choice,
`scripts/privacy_check.py`, packaging, `scripts/install_release_device.sh` (install and
installed-app test on the Cardputer), a report for approval, docs, and the merge into `main`.

## Build locally

Requires Python 3.9+, Git, a C/C++17 compiler, and Make. Dependencies and CMake
stay inside this project:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install cmake==3.31.6
python3 scripts/bootstrap.py
```

Copy the device's `/usr/share/sounds/sf2/TimGM6mb.sf2` into
`.tools/soundfonts/TimGM6mb.sf2` for local audio/tests, or pass another compatible
GM bank using `--soundfont PATH` / `ZEROCHORD_SOUNDFONT`. A substituted bank may
sound different and keeps its own license. The bank is not checked into Git.

On Debian, the `timgm6mb-soundfont` package supplies this bank at
`/usr/share/sounds/sf2/TimGM6mb.sf2`. You can link it into the project:

```sh
mkdir -p .tools/soundfonts
ln -s /usr/share/sounds/sf2/TimGM6mb.sf2 .tools/soundfonts/TimGM6mb.sf2
```

Run `./scripts/check.sh` for the daily edit and test loop. It configures and
incrementally builds the SDL preview, then runs all six CTest suites. The script
can be called by path from any directory. Set `CMAKE_BUILD_PARALLEL_LEVEL=2` if
you want to leave more memory available for other work.

From the guest's graphical desktop, launch the preview. If you have a local
`.env`, load it in the same subshell:

```sh
(
    if test -f .env; then
        set -a
        . ./.env
        set +a
    fi
    build/desktop/M5CardputerZero-ZeroChord
)
build/desktop/zerochord-render .tools/soundfonts/TimGM6mb.sf2 build/demo.wav
```

### Build for the device from the Debian VM

Copy [.env.example](../.env.example) to `.env` and set the SSH host, optional SSH
config, and device directory there. Load it before deploying:

```sh
set -a; . ./.env; set +a
.venv/bin/python scripts/build_device.py
sh scripts/deploy_device.sh
```

The first command installs checksum-verified Zig 0.15.2 inside `.tools/`, then
builds the Linux ARM64 framebuffer app into `build/cross-device/`.
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

## Releasing

Run the `/publish` workflow (`.claude/skills/publish/SKILL.md`). Update the docs
for the candidate, then finalize the docs and verify the repackaged archive
before requesting release approval. The workflow uses these tools:

- `scripts/privacy_check.py [--deb PACKAGE]` scans publishable files, commit
  metadata since the last tag and the package for private values from `.env`, Git
  configuration and the local account. Add extra terms with `ZEROCHORD_PRIVATE_TERMS`
  (comma-separated) in `.env`. FAIL blocks a release; WARN needs a human decision.
- `scripts/package.py` builds `dist/zerochord_<version>_arm64.deb`.
- `scripts/install_release_device.sh push|install|test` copies the package to the
  device, installs it through the workspace `cardputer-sudo` helper when using
  the workspace SSH profile, and runs `scripts/test_installed.py` against the
  installed app. Use `install-interactive` if the device needs a password prompt.
- `scripts/release_screenshots.py` uses the desktop build to refresh the dark-theme
  screenshots in `docs/images` and copies the five store images to `store/screenshots`.
- **Publish to GitHub** pushes `main` and the release tag, then creates a
  GitHub Release with the final `.deb`, SHA-256 file, and tagged release notes.
  `ZEROCHORD_GH_USER` in `.env` selects the account for `gh` commands.
- **Submit to the CardputerZero Store** using the [store listing](../store/LISTING.md)
  and record the submission in [PUBLISH.md](../PUBLISH.md).
- `scripts/install_git_guard.sh` installs a local pre-push hook that refuses `dev`.

## Sounds and front notes

User-facing behavior is documented in the [user guide](USER_GUIDE.md),
[front notes](FRONT_NOTES.md) and [custom sound packs](../sounds/README.md).

### Controlled sound comparison

Fetch the optional licensed Piano with `python scripts/fetch_piano_pack.py`.
Build `zerochord-compare` in the desktop build, then run:

```sh
build/desktop/zerochord-compare .tools/soundfonts/TimGM6mb.sf2 \
  sounds/fluid-grand/piano.sf2 build/audio-comparison
```

This checks the dry native renderer reference and creates matched-RMS WAVs.
See [audio notes](AUDIO_AND_REUSE.md#controlled-piano-and-renderer-comparison)
for what this comparison establishes and its limits.

### Style balance

`zerochord-styles` is built with the desktop build. It renders each built-in
style playing I–vi–IV–V with a front melody:

```sh
build/desktop/zerochord-styles .tools/soundfonts/TimGM6mb.sf2 build/styles [style]
```

It writes one WAV per style (only `[style]`, such as `dream`, when given) and
prints each layer's RMS and peak level, measured before the limiter, and the
share of its energy below 150 Hz, at 150–600 Hz, at 600 Hz–2.5 kHz, and above
2.5 kHz. Set `ZEROCHORD_LAB_FULL=1` to measure each layer's solo at 100% instead
of at its mix. `zerochord-styles <bank.sf2> --levels` prints each program's
measured loudness and its level-matching gain. Use it to set style mixes; it
does not replace listening.
