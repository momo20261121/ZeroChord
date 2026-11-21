# Testing ZeroChord

## Desktop first, hardware early

The [official CardputerZero template](https://github.com/CardputerZero/Template)
provides an SDL simulator for fast UI development and a separate Linux device
build. That supports a two-stage workflow; it does not establish that simulation
can validate keyboard rollover, speaker quality, or end-to-end audio latency.

ZeroChord separates its music state machine and offline audio renderer from
LVGL and physical devices. Use all three layers below.

### 1. Deterministic local tests

```sh
.venv/bin/ctest --test-dir build/desktop --output-on-failure
```

`musical_behavior` exercises 12 keys, major/natural minor roots, all combination
qualities, press/release stability, sharp, slash bass, repeat suppression,
momentary/held chords, sweep pitches/direction/timing, phase-preserving rhythm scheduling, simultaneous chord hits, all patterns, preset
changes/revoicing, bounded stall recovery, and panic cancelling queued sweeps.

`audio_rendering` uses the real sound bank: the four original sound pairs and nine complete factory presets must produce finite,
nonzero samples without clipping in the test sequence. It checks independent
releases of identical pitches, immediate panic silence, and a held chord surviving
100 harp triggers. It also plays all ten assignable chord qualities with their
seven harp notes through all four presets. These tests do not prove every possible performance is free
of clipping.

`settings_behavior` tests minichord defaults, no3/sus voicings, shared and
per-degree mappings, all scope shortcuts, inherited/custom behavior, preview
transactions, Enter/Esc, sound choices, reset confirmation, real performance
dispatch, persistence, legacy configurations, and invalid-value recovery.

`preset_behavior` tests complete TOML round-trips, file validation, legacy migration,
per-preset working copies and stable selection, all musical defaults, individual
Backspace reset and picker cancel, app-wide volume/theme preservation, save-as,
edited factory defaults, and no-save behavior.

Audio regressions additionally verify simultaneous hits have a falling envelope,
master × preset gain scales direct sound and echo, and pad motion preserves the
foreground. Panic must discard previous voices before the next note unmutes output.

AddressSanitizer and UndefinedBehaviorSanitizer can be enabled on the standalone
engine build:

```sh
.venv/bin/cmake -S . -B build/sanitize -DZEROCHORD_GUI=OFF \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
.venv/bin/cmake --build build/sanitize -j4
.venv/bin/ctest --test-dir build/sanitize --output-on-failure
```

### 2. Desktop UI and rendered sound

Run the SDL app to inspect the same 320 x 170 LVGL screen at 3x size. This is a
desktop preview, not a CPU or hardware emulator. Test scripts enter musical keys
through the app's normal input dispatcher. Native-resolution screenshots use PPM:

```sh
build/desktop/M5CardputerZero-ZeroChord --headless --no-audio --no-save \
  --script tests/performance.events --quit-after-ms 1700 \
  --capture build/performance.ppm
python3 scripts/ppm_to_png.py build/performance.ppm build/performance.png
build/desktop/zerochord-render .tools/soundfonts/TimGM6mb.sf2 build/demo.wav
```

`--no-audio` is explicit: normal launches fail with a diagnostic if audio cannot
start. `--no-save` prevents test settings from overwriting user settings; tests
that need known defaults should also use an isolated `XDG_CONFIG_HOME`.

### 3. Native device validation

Build on Linux AArch64 with a project-local CMake (or an existing installation):

```sh
.tools/cmake/bin/cmake -S . -B build/device -DZEROCHORD_DESKTOP=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DZEROCHORD_TEST_SOUNDFONT=/usr/share/sounds/sf2/TimGM6mb.sf2
.tools/cmake/bin/cmake --build build/device -j1
.tools/cmake/bin/ctest --test-dir build/device --output-on-failure
python3 scripts/device_input_test.py
```

For development from macOS, `scripts/prepare_device.py` makes an ignored source
archive using a Linux ARM64 CMake wheel downloaded into `.tools/downloads`:

```sh
.venv/bin/python -m pip download --no-deps --only-binary=:all: \
  --platform manylinux2014_aarch64 cmake==3.31.6 -d .tools/downloads
python3 scripts/prepare_device.py
```

Copy the resulting `.tools/zerochord-device-source.tar.gz` to the device and
extract it in a user-owned development directory. The bank stays supplied by
the device's `timgm6mb-soundfont` package. The bundle includes project-local CMake,
LVGL, miniaudio, TinySoundFont, and toml++, but excludes macOS tools and SDL.

The integration test creates a temporary uinput keyboard and passes its event
path only to ZeroChord. It stops the user's `APPLaunch.service` for display
ownership and always restarts it on completion/failure. It exercises real evdev
delivery, framebuffer output, the PipeWire PulseAudio service, combinations,
strums, arp, Space, settings, short/long Esc, and SIGINT. It does not replace the
physical keyboard's by-path symlink. Run only when interrupting the launcher is
acceptable, with no other app playing. User must have access to input/video/audio.

The editor regression uses real evdev arrow/Enter/number events to set shared
no3 and V-only sus4, cancel a candidate change, and return to performance. It also
saves/reloads custom mappings and complete presets in an isolated configuration directory, preserving
the user's own settings. Kernel framebuffer captures include the combination
editor and picker. Fixture scripts accept UP/DOWN/LEFT/RIGHT/ENTER as named keys.

The test also starts the preview with backlight brightness zero, checks that it
wakes, and reads the kernel framebuffer independently of LVGL. It compares this
readback with the rendered performance screen within RGB565 quantization error.
The app's `--capture` alone captures its internal render buffer and cannot prove
that the panel is lit. Even framebuffer readback and backlight state need a
physical visual check to confirm the panel actually displays the expected UI.

To run a development preview over SSH, use `scripts/run_device_preview.sh`
instead of stopping the launcher and invoking the binary directly. It wakes a
sleeping backlight while preserving nonzero brightness, then restores APPLaunch
on exit. The framebuffer backend also requests `FB_BLANK_UNBLANK` on startup.
To recover a failed preview, stop its user service and start the launcher:

```sh
systemctl --user stop zerochord-preview.service
systemctl --user start APPLaunch.service
```

The framebuffer backend honors `LV_LINUX_FBDEV_DEVICE`, otherwise prefers
`st7789`/`panel-mipi` devices and validates 320 x 170 geometry before mapping.
The verified development unit uses `/dev/fb0` (`panel-mipi-dbid`, RGB565); this is
detected rather than hard-coded. `ZEROCHORD_INPUT_DEVICE` overrides the default
keyboard for diagnostics. `SYN_DROPPED`, focus loss, and disconnect cause panic.

## Install and launcher checks

The package installs its desktop entry, wrapper, binary, and icon under
`/usr/share/APPLaunch`. Installation needs root. The current development unit
requires an interactive sudo password; do not put that password in scripts or
chat. Install the prepared package in a device terminal, then restart the user's
launcher service:

```sh
sudo dpkg -i ~/ZeroChord/dist/zerochord_0.1.0_arm64.deb
systemctl --user restart APPLaunch.service
```

Confirm the ZeroChord entry and icon appear, launch it, play a chord and harp,
then hold Esc to return. Also verify the launcher's Home forced-quit behavior.
If a dependency is missing, install `timgm6mb-soundfont` and `libpulse0` using the
device's normal package manager, then retry. The app does not install them itself.

## Hands-on acceptance

These checks need the actual keyboard and listening; injected events cannot
substitute for them:

- Play 8–5–T–4 while using Z–M, K/L, and presets. Confirm direction and comfort.
- Hold 8+U+J (augmented), then 1+8+U+J; play harp keys simultaneously. Check
  physical rollover/ghosting and that every release arrives.
- Play slash chords and rolled combinations. Judge base-column precedence and
  whether the brief intermediate chords feel acceptable.
- Compare all sounds on speaker and headphones. Adjust balance/echo, play dense
  passages for several minutes, and listen for clipping, clicks, or dropouts.
- Measure input-to-output latency with a loopback/contact recording or an
  oscilloscope if a numeric latency target is needed. Callback count and queue
  overflow metrics alone are not end-to-end latency measurements.
- Exit/relaunch to check saved settings and silent startup. Test short Esc in
  settings, long Esc, Home, and unexpected audio/input disconnection.

## Complete preset update — 2026-11-21

- Four CTest suites pass on desktop Release, desktop ASan/UBSan, and native
  CardputerZero AArch64 Release: musical behavior, settings, complete presets,
  and real SoundFont audio rendering.
- All nine factory styles render audible, finite audio; all rhythm choices
  exercise the actual scheduler. Tests cover simultaneous Chord hits, falling
  amplitude, phase continuity across harmony/tempo changes, bounded stall
  recovery, independent foreground sound, and master × preset gain including echo.
- TOML tests cover full round-trips, schema/range/type/name validation, malformed
  files, stable IDs, legacy migration, custom defaults, remembered edits, field
  and whole-preset restores, globals surviving preset changes, save-as file
  collision avoidance, and no-save behavior.
- Real evdev integration passes shared no3/V-only sus4 editing, picker cancel,
  Backspace field reset, BPM preservation, mode and all part-pattern edits,
  save-as User001, preset switching/recall, complete restore, global master/theme
  preservation, clean exit and reload. Configurations are isolated from the
  user's setup. Native framebuffer captures show both themes and preset choices.
- Independent kernel framebuffer readback matches the internal rendered frame
  within RGB565 quantization error. Backlight wake, short/long Esc, SIGINT, and
  return to APPLaunch pass.
- PulseAudio output capture contains **1,881,600 stereo frames**, peak **8,953 /
  32,768**. Every app run reports **zero audio command-queue overflows**.
  Dream at 100 BPM with echo disabled has peak 10 ms mean-square energy
  **1,416,549.19** in Pad, **0** after the Manual Chord hit has decayed, and
  **0–67,059.03** in Arp. This verifies digital output and timing gaps; it does
  not measure acoustic quality, physical key rollover, or end-to-end latency.

![Complete preset settings in the light theme](images/light-settings.png)

![Saved user preset with its own mode and tempo](images/presets.png)

![Simultaneous chord accompaniment using a saved preset](images/preset-performance.png)

## Earlier prototype evidence

2026-11-21, before complete presets (the earlier middle mode was called Strum):

- Desktop engine and renderer tests passed in Release and with ASan/UBSan.
- The settings/persistence test suite also passed in Release and with ASan/UBSan.
- Actual SDL window launched and exited successfully; main/settings screenshots
  were checked at 320 x 170.
- The real desktop audio callback ran with zero command-queue overflows; saved
  settings were checked across a restart in an isolated configuration directory.
- Native AArch64 Release build succeeded; all three CTest tests passed on device.
- Device integration passed with the physical framebuffer and a temporary evdev
  keyboard: combinations, K/L, arp, Space, settings, short/long Esc, and SIGINT.
  The latest PulseAudio output monitor captured 1,149,600 stereo frames with nonzero
  signal (peak 8,515 / 32,768); each app run reported zero command-queue
  overflows. This verifies signal reached the audio service, not acoustic quality
  or absence of every possible dropout. The launcher was restored and active.
- The performance image below comes from independent kernel framebuffer readback;
  the settings image comes from the native app's internal render buffer. Neither
  image alone verifies physical panel visibility.
- An SSH preview exposed a missed sleeping-backlight case (brightness zero).
  The launcher was restored; preview startup now wakes the backlight, and the
  regression test checks wake behavior and independent kernel framebuffer output.
- The Debian archive was parsed; desktop entry, executable wrapper, stripped
  AArch64 binary, PNG icon, dependencies, and license notices were verified.
- System launcher installation: pending interactive sudo authentication.
- Physical rollover, listening tests, and measured end-to-end latency: pending.
- Customization update: native/desktop/sanitizer suites passed, along with real
  evdev shared no3/V-only sus4 edits, picker cancel, and isolated save/reload.
- Playback update: desktop, native, and ASan/UBSan suites cover the O cycle,
  sustained Pad, a single four-note Strum, timed Arp notes, and manual harp
  sweeps alongside Strum. All four sound presets pass short-note attack and
  quiet-gap regressions. The actual Dream control path verifies sustained Pad,
  Strum falling silent, and three separate Arp pulses with quiet gaps.
- Live Dream output was also measured through the device's PulseAudio sink
  monitor with echo disabled in an isolated test configuration. Ten-millisecond
  windows had peak mean-square energy 1,489,405.73 for Pad, zero for Strum after
  its sweep ended, and 0–185,000.21 for Arp. The recorder requests 50 ms latency;
  measurements allow the output/capture buffers to settle. This verifies the
  electronic output, while speaker/headphone listening remains a hands-on check.

![Native device performance screen](images/performance.png)

![Kernel framebuffer: Arp playback mode](images/arp.png)

![Native device settings screen](images/settings.png)

![Kernel framebuffer: per-degree combination editor](images/combinations.png)

![Kernel framebuffer: auditioning an inherited choice](images/chord-choices.png)

## Rebuild the package

On the device, use the CardputerZero application skill's packaging helper
(`make_applauncher_package.py`) as the staging contract. Copy it into `.tools/`
for this development-only command; it is not part of the installed package:

```sh
python3 scripts/package.py --helper .tools/make_applauncher_package.py
dpkg-deb --info dist/zerochord_0.1.0_arm64.deb
dpkg-deb --contents dist/zerochord_0.1.0_arm64.deb
```

The packaging script uses staging-only helper flags. Neither `--install-local`
nor `--auto-install-cardputer` is passed; package creation does not install system
files or restart the launcher. It rejects non-AArch64 binaries, strips the staged
binary, retains license notices, and produces `dist/zerochord_0.1.0_arm64.deb`.
Requires Python 3.9+, GNU `strip`, and `dpkg-deb` on the packaging host.
