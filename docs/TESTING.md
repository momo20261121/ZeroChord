# Testing ZeroChord

## Everything in ~/ZeroChord — 2026-11-27

- **Local:** all eight suites and 47 real-app flows pass. `settings_tests` covers the
  move from the old folder (settings, presets, loop and packs arrive; packs already in
  `~/ZeroChord/sounds/` stay; the emptied old folder is deleted; nothing moves twice;
  a clashing file stays in the old folder; a failed move is undone and the old folder
  used) and `ZEROCHORD_HOME` isolation. The new `moved-to-zerochord` flow runs the real
  app with a scratch home and checks the result. Every script now isolates its runs
  with `ZEROCHORD_HOME`.
- **Device:** the development folder moved to `~/zerochord-dev`. The deployed build's
  first launch moved the device's settings, presets, loop and its three FluidR3 packs
  into `~/ZeroChord/` and removed `~/.config/zerochord/`. With a scratch
  `ZEROCHORD_HOME`, a plain `.sf2` (the system TimGM6mb bank, linked as
  `my-sounds.sf2`) loaded with no warnings beyond the 64-sound note and appeared under
  **Front sound → Custom**, checked at 320×170. Playing it was not listened to.

## 3/4 loops, tap tempo, Shortcuts, MIDI bend — 2026-11-27

- **Local:** all eight desktop suites and 46 real-app flows pass. `loop_tests` checks the
  meter rule (each of the four 3/4 patterns makes a style 3/4, muted or not), a Waltz
  take ending on its second 3/4 bar, three count-in clicks a bar, wrapping after 24
  16ths, the meter surviving save and load (older files read as 4/4), and a held bend
  and a mod wheel move recorded and replayed on the loop's slot. `audio_tests` measures
  rendered pitch: a full bend raises a MIDI note two semitones, before or during the
  note, while backing notes, the other slot's notes and a reused voice channel stay
  in tune; the mod wheel wobbles the pitch and stops when returned to 0. Each of these
  checks was confirmed to fail with the feature disabled. `midi_tests` covers parsing
  14-bit bends and controller handling; `settings_tests` covers tap timing, the pause
  reset and the 240 BPM limit, and walks the Shortcuts tab as read-only: every page
  fits six lines, any arrow turns the page and wraps, other keys change nothing.
- **UI:** all six Shortcuts pages and the light theme, the Tempo page with taps and with Waltz's
  3/4 time, the 3/4 recording light and count-in were checked at 320×170.
- **Not yet:** a real MIDI keyboard's bend and wheel, and listening on the device.

## Loop — 2026-11-27

- **Local:** all eight desktop suites pass, including the new `loop_tests`. It covers
  clean-up (short chords merged on the raw times before snapping, independent snapping
  over 128 changes with no drift, stabs kept, short gaps closed, collisions, changes
  and chords at the loop's end), recording (arming, the take starting at the first
  key of a combination, ending on the nearest bar, a late Enter within the window,
  the length limit, dropping a take), playback (slot 1 commands, the loop chord on the
  screen and Z-M, a live chord on top, tempo following, Space), Recorded vs Follow
  current, Loop mix overrides and restore, and save/load. `audio_tests` checks that a
  live Panic or chord stop leaves loop notes sounding and stopping the loop leaves live
  notes. `settings_tests` walks the Loop page. All 44 real-app flows pass.
  The count-in is tested too: four clicks, beat 1 accented, then beat 1, and Enter
  cancels it.
- **UI:** the waiting, recording, holding, count-in and playing lights, the loop chord
  with and without a live chord, All front with a loop, the Loop and Loop mix pages and
  the delete question were checked at 320×170, in both themes for the playing screen.
- **Device load** (`scripts/device_load_test.py`, with the project `.env` loaded): the
  deployed app runs a scripted Dance session (all layers, 124 BPM) with isolated settings
  and the installed sound packs, sampling `/proc` every 0.25 s. It fails if the busiest
  thread passes 70% of one core or the audio queue overflows. `--local` dry-runs it on the
  desktop build. Results on the Cardputer (4 cores), 2026-11-27:

  | Phase | Process | Busiest thread | RSS |
  | --- | --- | --- | --- |
  | Idle | 19% | 11% | 65 MB |
  | Live playing while recording | 45% | 31% | 65 MB |
  | Count-in | 27% | 14% | 65 MB |
  | Loop alone | 46% | 32% | 65 MB |
  | Loop + live chords, strums and front notes | 50% | 38% | 65 MB |
  | Loop + All front runs | 52% | 37% | 65 MB |

  No audio queue overflows. Without the sound packs, RSS is 19 MB and each phase about
  2 points lower.

## All front — 2026-11-27

- **Layout:** the device keymap (`tca8418` `linux,keymap`) puts `4`–`0`, `E`–`O`, `D`–`L`
  and `Z`–`M` in matrix columns 2–8, an aligned 7×4 block. The screen draws it aligned.
- **Local:** all seven desktop suites pass. `music_tests` covers both layouts (gaps, short
  scales, own key, Lowest note clamping to white keys, the octave range), playing, `1`,
  `Q`/`W` limits, the chord kept with Hold and stopped without it, Space and
  leaving. `settings_tests` covers the page, the shared Key/Scale, white-key
  stepping, restore and persistence. All 44 real-app flows pass, including a new All
  front flow. The in-place All front screen matches a fresh drawing in both modes, and
  holding Space returns to the chord screen.
- **UI:** both modes and the settings page were checked at 320×170.

## MIDI input — 2026-11-27

- **Local:** all seven desktop suites pass, including the new `midi_tests`. It covers
  parsing (running status, note on at velocity 0, split reads, SysEx, system common
  and real-time bytes), each Plays target, velocity curves, the sustain pedal,
  restarting the same key, the 16-note limit, `Space`, All notes off and unplugging.
  `audio_tests` checks that a chord stop leaves a MIDI note sounding.
  `settings_tests` covers the page, restore, the app-wide setting across presets,
  and persistence. All 43 real-app flows pass.
- **UI:** the App row, MIDI input page, Plays list and lit playing-screen badge
  were checked at 320×170 in the headless preview. A FIFO named `midiC9D0` in
  `ZEROCHORD_MIDI_DIR` stood in for a raw MIDI port and fed a note.
- **Device:** cross-built and launched; the device supports USB host mode and has
  `snd-usb-audio`. Not yet tested with a real keyboard: detection, unplugging
  (ENODEV), bus power, and note latency.

## 1.4.0 release — 2026-11-26

- **Local:** all six desktop suites, 43 real-app flows and the playing-screen check passed. The
  tempo-swing flow was redone for the Swing and Swing amount rows, and output-speaker
  is new. New tests cover the Close, Open, Power, and Rich voicings, voice
  leading between chords, the bass taking the root from Chord and Pad, sound
  level matching, 8ths and 16ths swing, and the upgrade from the second 1.4
  development build, including its old `swing` key.
- **Device:** cross-built and deployed. With all layers playing, Dream uses
  about 30% of one CPU core, Pop 39%, and Dance 42%, with 66 MB RSS and no
  audio queue overflows.
- **Screenshots:** regenerated with `scripts/release_screenshots.py` and
  checked at 320×170.
- **Latency (device, scripted playing with chords, front keys, strums and
  preset changes):** before, each chord or front-key redraw took 60–90 ms
  (widget rebuild 36–54 ms, render 28–31 ms) and the main loop averaged 8 ms of
  work; keys waited up to 96 ms. After the in-place playing screen, the loop
  averages 0.5 ms; only preset changes still rebuild the screen (36–53 ms).
  With the 128 × 3 audio request, callbacks arrive every 2.7 ms (gaps at most
  3.9 ms), each renders in 0.7–2.8 ms, and PipeWire counted no xruns. A new
  flow checks that the playing screen after busy playing matches a fresh
  drawing in both themes.
- **Release run:** all six desktop suites, all six ASan/UBSan suites (leak
  detection off), 43 real-app flows and the playing-screen check passed. Because
  the playing screen now keeps its widgets between redraws, the desktop app was
  also built with ASan/UBSan: the same flows passed with no reports, and 13
  playing-screen states matched the 1.3-style full redraw pixel for pixel.
- **Installed test:** `zerochord 1.4.0` installed over 1.3.0 on the
  CardputerZero; `scripts/test_installed.py` passed (version, `dpkg --verify`,
  launcher entry, injected evdev keys, settings saved and reloaded, kernel
  framebuffer matching the app capture, clean long-Esc exit). Digital capture
  recorded 424,800 stereo frames with peak 29,164 and no queue overflows. The
  test now also expects Bass and Drums to stay on after muting Arp and Pad, and
  its comments describe the 1.4 Backing layers page.
- **Screenshots:** `scripts/release_screenshots.py` reproduced the committed
  1.4 images byte for byte; the store set was inspected at 320×170.
- **Final package:** repackaged after the installed test to date the changelog
  and add this record. Outside `usr/share/doc/zerochord/` its files and modes
  match the tested package, and only `Installed-Size` changed in the control
  fields, so the installed test was not repeated. The final package was
  installed on the device with a clean `dpkg --verify` and the same executable.
- **Not yet done:** listening, a hands-on check that Auto detects the headphone
  jack correctly, and the feel of the lower latency with physical keys.

## 1.3.0 release — 2026-11-25

- **Local:** all six desktop suites, all six ASan/UBSan suites (leak detection
  off), and 37 real-app flows passed. New coverage includes held front notes and
  their highlight, the Backing layers page with live mute and mix, separate Arp
  sound routing and mix gain, 1.2 settings and preset migration, the chord Scale
  and front Key/Scale following, the envelope list's sound value, hold-to-repeat,
  Tab and number tab navigation, one-question confirmations, and previews
  stopping when the player leaves an option. The keyboard fixtures, flows, and
  installed acceptance sequence were updated for the new controls.
- **Fix during release:** leaving Backing layers opened with O changed the Style
  row that Tab opened next. O now leaves the settings pages untouched
  (`settings_tests`); the installed acceptance test found it.
- **Package:** the Debian VM cross-built the Linux ARM64 candidate with Zig. The
  CardputerZero verified the candidate checksum before installation. The final
  archive uses the same executable and launcher files with updated docs.
- **Device:** the installed test passed package integrity, launcher metadata,
  evdev input, save and reload of App Vol, EQ, a Chord-only layer set, chord
  pattern, and tempo, and long-Esc exit. The kernel framebuffer matched the
  320×170 app capture within RGB565 quantization. Digital audio captured 424,800
  stereo frames with peak 9,271 and zero queue overflows. The launcher resumed.
- **Screenshots:** inspected the five store screenshots, the App settings
  screenshot, and the new Backing layers screenshot at 320×170.
- **Manual checks:** physical keyboard feel (holding notes and repeating
  arrows), speaker or headphone listening, the 30% join level for added layers,
  and optional sound packs remain outside the automated acceptance test.
  `scripts/device_input_test.py` still walks the 1.2 menus and was not run.

## 1.2.1 release — 2026-11-24

- **Local:** all six desktop suites, all six ASan/UBSan suites with leak
  detection, and 28 real-app flows passed. Coverage includes direct volume and
  EQ edits, Hold and the 120 ms chord window, persistence, and long Esc. The
  audio suite verified linear App Vol gain with EQ and Echo enabled.
- **Package:** the Debian VM cross-built the Linux ARM64 candidate with Zig.
  The CardputerZero verified the candidate checksum before installation. The
  final archive uses the same executable and launcher files with updated docs.
- **Device:** the installed test passed package integrity, launcher metadata,
  evdev input, save and reload of App Vol, EQ, backing layer, chord pattern, and
  tempo, and long-Esc exit. The kernel framebuffer matched the 320×170 app
  capture within RGB565 quantization. Digital audio captured 420,000 stereo
  frames with peak 10,381 and zero queue overflows. The launcher resumed.
- **Screenshots:** inspected all five store screenshots and the App settings
  screenshot at 320×170.
- **Manual checks:** physical keyboard feel, speaker or headphone listening,
  and optional sound packs remain outside the automated acceptance test.

## 1.2.0 release — 2026-11-22

- **Local:** all six desktop suites, all six ASan/UBSan suites (macOS leak
  detection off), and 27 real-app flows passed. The flows cover the Volume
  & effects subpage, printed arrow navigation, save/restart, and long Esc.
- **Package:** the ARM64 candidate was cross-built on the Mac and transferred
  to the CardputerZero with its checksum verified. The privacy scan found zero
  failures and flagged the public About link in the source and executable.
  The release package adds the final documentation and keeps the tested
  executable, launcher entry, wrapper, and icon byte-for-byte.
- **Device:** the installed test passed version and package integrity checks,
  launcher metadata, evdev input, and isolated save/reload of backing layer,
  chord pattern, EQ, and tempo. The 320×170 framebuffer matched the app capture
  within RGB565 rounding. The digital recording captured 420,000 stereo frames
  with peak 9,954 and zero audio queue overflows. Long Esc exited cleanly,
  APPLaunch resumed, and the installed executable matched the cross-build.
- **Screenshots:** all six documentation screenshots were reviewed at 320×170.
  Five of them are also in the store set.
- **Manual checks:** physical keyboard feel and speaker or headphone listening.
  The optional Grand Piano pack was outside the installed-device test.

## 1.1.0 release candidate record (2026-11-22)

- **Local:** all six desktop suites, all six ASan/UBSan suites (macOS leak
  detection off) and 26 real-app keyboard/save/restart flows pass. The sanitizer run
  preceded the final About-tab text edit, which changes one UI label only; the
  desktop suites and flows were rerun after it.
- **Package:** `dist/zerochord_1.1.0_arm64.deb` is cross-built on the Mac. The privacy
  scan reports no FAIL; its one WARN is the intentional public project URL in the About QR.
- **Device:** the package was installed on the CardputerZero and `scripts/test_installed.py`
  passed (installed version, `dpkg --verify`, launcher entry, evdev input, 415,200
  recorded audio frames with peak 9,116 and no queue overflows, framebuffer readback,
  persisted Chord-only layer/pattern/EQ/tempo, clean exit). The installed executable's
  SHA-256 matches the local build.
- **Script fix:** the 1.0.0 installed-app script used stale popup keys and expected a
  non-default pattern; it now selects Chord only and the next pattern. The desktop app
  produced identical results, so this was a script issue, not an app bug.
- **Not validated:** physical keyboard feel, speaker listening, the optional Grand Piano
  pack on the device, and the other hardware scripts (`scripts/device_input_test.py`).
  The About screen was inspected in the 320×170 desktop render, not on the device display.

Sections below are dated iteration records, newest first; their counts describe
those versions.

## 1.1.0 backing layers and preset tuning (iteration record)

All six local suites pass, including all eight layer combinations, checkbox
apply/cancel/default, app and preset round-trips, current-preset restore,
independent Front/Chord/Arp/Pad rendering, Pad-only motion, panic and normal-level
clipping checks. The real app passes 20 isolated keyboard/save/restart flows,
including O and Style checkbox paths using printed F/X arrows, none/all
selections and cancellation. The 320×170 popup and multi-layer status are
visually checked locally. Cross-compilation runs on the development Mac.
This iteration's device check was one preview deployment and launch.
See the 1.1.0 release candidate record above for the installed-device results.

## Global EQ and sustained chord pattern

All six desktop suites and all six ASan/UBSan suites pass locally (macOS leak
sanitizer disabled). Tone tests at 44.1/48 kHz measure the Low/Mid/High bands at
−12, −6, 0, +6 and +12 dB, neutral bypass and stereo independence. Real SoundFont
rendering verifies EQ processes the full dry/echo mix, Master mute still works,
and panic clears filter tails. Sustained Chord tests cover a prompt attack,
audible held body after two seconds, release with Hold Off, latch with Hold On,
chord replacement, panic, and preset pattern round-trip. Existing pattern IDs
and defaults are retained; EQ survives style changes, restore and reload.

The real app passes 15 isolated save/restart flows using printed F/X/Z/C keys,
including EQ edit/cancel/restore and choosing Sustained. Dark/light EQ layouts
are inspected at 320×170. Only the local ARM cross-build and one deployment/launch
check follow these local checks; no second full test suite runs on the device.
Speaker bass response and preferred EQ amounts remain listening judgments.

## Menu navigation audit — 2026-11-21

Physical printed arrows F/X/Z/C and dedicated arrow events now use the same
navigation in every settings page and picker. Previously the Front notes and
Chord combinations pages accepted only dedicated arrow events, leaving the
device's printed arrows unresponsive. Local coverage walks all Style/App rows,
both front modes and every key tab, and every chord scope, including wraparound,
sound categories, preview isolation, Enter commit, Esc cancel, Backspace default,
and Tab closing a pending edit. The main view separates each note plus octave
from its keyboard label, checked locally at 320×170 in dark and light themes.

## Front editor and combination recognition — 2026-11-21

The local regression checks cover arrow-only front tabs, All/per-key restores,
actual-octave migration, default layout preservation, chord/scale modes, per-mode
K/L actions/times, octave bounds, and settings persistence across style changes.
Combination recognition covers all row-button orders, a fixed first-press
deadline, staggered three-button presses, late additions, immediate front notes,
release stability, cancellation, and preserved rhythmic phase. The 30 ms trial
default is adjustable; no hardware action-to-sound latency has been measured.

## macOS cross-build — 2026-11-21

The device app now builds locally with project-local Zig 0.15.2, targeting Linux
ARM64 with glibc 2.36 compatibility. The first full build, including the compiler
download, took 41.3 seconds on the development Mac. The produced ELF was copied
to the CardputerZero with checksum verification and launched successfully with
the framebuffer and normal audio enabled. No compilation or repeated test suites
ran on the device. The previous native binary is retained for rollback.

## Named sounds, packs, and front notes — 2026-11-21

Local coverage adds curated/category picker transactions, manifest validation,
real SF2 pad/chord/front routing, missing-pack fallback, stable IDs and preset
round trips, front-key editors/reset/cancel/persistence, chord-aware versus fixed
intervals, actual octaves, separate chord/scale layouts, and matching pluck/strum pitches. Five local
suites pass. Both new editors are checked at 320×170 before one local ARM64 cross-build.
A successful device audio launch is checked; the local suites are not rerun there.

## App controls and menu order — 2026-11-21

Volume, echo, and sweep are app-wide; preset level is removed. Style starts
with Preset then Tempo; App starts with Theme. All four local suites pass,
including app defaults, preservation across preset selection/restore/restart,
legacy preset compatibility, and master scaling of dry and wet audio. The
local 320×170 layout is checked before one native app build and launch; no
repeat device test suite or package rebuild is needed for this iteration.

## Separate sounds and Front only — 2026-11-21

Pad sound and Chord sound are separate preset fields; Arp uses Chord sound.
Older presets inherit their former backing sound for Pad. Local checks cover
independent audio routing with the real SoundFont, foreground independence,
per-field reset, preset round-trip, and legacy fallback. Front only tests verify
silent chord selection, retained harmony, foreground notes/sweeps, mode cycling,
and persistence. All four local suites passed before the device build. The
updated native app was reopened; repeated device test suites and packaging
were skipped for this iteration. The previous packaged release remains unchanged.


## Tabbed settings update — 2026-11-21

Settings now have Style and App tabs. Left/Right switches tabs without changing
values; Up/Down selects rows and Enter opens a transactional editor. Each tab
remembers its row. Preset is first on Style. Hold retains its existing name.

App-wide key/scale, Hold, chord combinations, master volume, and theme survive
preset selection, save-as, restoration, and restart. Style presets retain BPM,
sounds, patterns, levels, echo, and sweep timing. Version 1 files remain readable;
new version 2 preset files omit app fields. Tests cover the upgrade with a legacy
working snapshot whose app values differ from the active settings.

The four desktop and native suites passed. Both tabs were visually checked at
320×170. A focused real-evdev check passed for tab switching, Enter/cancel, tempo
editing, preset recall, global settings persistence, restart, and framebuffer
capture. The broad device audio integration suite was not repeated for this UI
change. The updated preview was reopened with the user’s setup preserved.


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

Audio regressions additionally verify prompt chord attacks, finite gates,
flowing arp continuity, master gain on direct sound and echo, and Pad motion
preserving Front, Chord and Arp. Panic must discard previous voices before the next note unmutes output.

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

### 3. Cross-build locally, validate the launch on device

After running `./scripts/check.sh`, load the device settings and build from the Debian VM:

```sh
set -a; . ./.env; set +a
.venv/bin/python scripts/build_device.py
sh scripts/deploy_device.sh
```

Zig and its caches stay in `.tools/`; the ARM64 binary and incremental objects
stay in `build/cross-device/`. Deployment uses the existing SSH host and wrapper,
checks the copied binary's SHA256, saves settings by stopping the old preview
cleanly, preserves its binary, and launches the new one. It verifies the service
and child process after two seconds and restores the old binary if startup fails.
The sound bank remains supplied by the device's Debian package. The installed
APPLaunch package is separate from this development preview.

Normal iterations require no on-device compilation or repeated local suites.
Use focused hardware integration only for an observed regression or a specific
physical input, framebuffer, audio backend, or lifecycle risk.

#### Native compilation: diagnostic fallback only

If deliberately investigating a compiler difference on Linux AArch64:

```sh
.tools/cmake/bin/cmake -S . -B build/device -DZEROCHORD_DESKTOP=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DZEROCHORD_TEST_SOUNDFONT=/usr/share/sounds/sf2/TimGM6mb.sf2
.tools/cmake/bin/cmake --build build/device --target ZeroChord -j1
```

The device has about 352 MiB usable RAM. Parallel C++ compilation can exhaust
memory and stall SSH; do not use it as the normal development build host.

#### Focused hardware integration

Run `python3 scripts/device_input_test.py` on the device only when that broader
check is justified. It needs the development test fixtures on the device.

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
`/usr/share/APPLaunch`. Use `scripts/install_release_device.sh install` with
`.env` loaded. The script uses the workspace `cardputer-sudo` helper on the
configured Cardputer and restarts the launcher. To install manually in a device
terminal, run the following commands with the prepared package:

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

The original preset and prototype screenshots are preserved in `docs/images`
at tag `v1.1.0`.

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
- The performance capture used independent kernel framebuffer readback.
  The settings capture used the native app's internal render buffer.
  Physical panel visibility requires a separate check.
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

## Rebuild the package

Follow the [local cross-build and release instructions](DEVELOPMENT.md#releasing).
The package version comes from `VERSION.txt`. The installed-package acceptance
test runs separately on the CardputerZero.

### Tempo, tabs and preset isolation (2026-11-21)

The local menu audit covers both dedicated arrows and the physical F/X/Z/C
arrows across Style, App, all front tabs, all chord scopes, and sound groups.
Numeric tempo tests cover shared P/menu entry, save/cancel, deletion, range
validation, preset default, and unchanged audio transport. Preset tests edit two
styles, restore one, and reload to verify the other survives. Dream stock
migration preserves customized defaults and working snapshots. Layouts are
rendered locally at 320×170 for both themes and every tab layout, including
the tempo popup and performance hints. The ARM64 binary is cross-built locally
and deployed once, with only a launch check on the device.

## Envelopes and chord configuration

Local regression coverage checks independent Front/Chord/Pad ADSR routing,
attack and release timing, decay/sustain level, release on StopChord, optional
TOML validation/persistence, and preset-scoped restore. The menu audit walks all
controls with dedicated arrows and physical F/X/Z/C arrows, exercising preview,
save, cancel and default restore. Chord checks cover octave inheritance, absolute
roots staying fixed across key changes, root/scale independence, sharp, MIDI
bounds, settings persistence and preset operations preserving App settings.
Layouts are captured locally at 320×170 before one ARM cross-build and deploy;
no full second test suite is run on the device.

This batch passed all five local suites, and the changed pages were inspected in
both themes at 320×170. The Mac ARM64 cross-build completed in six seconds;
the transferred binary passed the preview service/process launch check.

## Broad physical-key scenario audit

See [the scenario review](SCENARIO_REVIEW.md) for the 80% backing default,
reproduced bugs, physical Z/C/F/X workflows, rendered-audio checks, sanitizer
results, design observations and local reproduction commands.
