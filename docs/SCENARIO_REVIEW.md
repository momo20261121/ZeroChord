# ZeroChord scenario review — 2026-11-21

*A dated review record, not current user documentation; see the [user guide](USER_GUIDE.md).*

Backing volume now defaults to **80%**; Front remains **100%**. Existing saved
volumes are preserved. Backspace on App → Volume & effects → Backing volume restores 80%.

## Bugs reproduced and fixed

1. **Front volume preview used the backing layer.** Editing Front volume now
   auditions a front note, including when the playing mode is Front only.
   Editing Backing volume in Front only now temporarily auditions a chord;
   cancelling preserves the saved mode and volume.
2. **Very high chord-octave auditions generated invalid MIDI notes.** Their
   extra front preview note is now bounded to MIDI 0–127. Custom root/octave,
   sharp and low slash-bass bounds are covered too.
3. **Slow Front envelopes were cut off during playing.** The old 1.6-second
   note gate could end a 3-second attack before it developed, although the
   editor preview worked. Playing now allows the explicit Attack + Decay plus
   300 ms, with the original 1.6-second minimum. Default envelopes are unchanged.
   A rendered Organ test reproduces the old silence and verifies the fix;
   Space still immediately silences a long envelope.

## Local checks

- All six CTest suites: musical behavior, settings, presets, rendered audio,
  sound packs/front mappings, and physical-key scenarios.
- **1,710 performance scenarios:** nine presets × all available mode/pattern
  combinations × ten chord qualities. Includes 40/240 BPM, 0/30 ms recognition,
  Hold on/off, sharp, all chord columns, extreme backing octaves, released
  combinations and front playing over accompaniment.
- **60,000 deterministic mixed playing events:** rolled/repeated presses and
  releases, Z–M, K/L, Hold, mode/preset switches, and panic.
- **40,000 deterministic menu actions:** physical **Z/C** for left/right and
  **F/X** for up/down, Enter, Backspace, Esc, Tab, number scope shortcuts and
  tempo typing. Checks selection/picker validity after each action.
- Existing menu audits walk every field, degree scope, front-key tab, sound
  category and envelope control using both physical aliases and dedicated
  arrows. Covers preview/save/cancel, individual/scoped restore, inheritance,
  preset isolation, saved edits and malformed configuration handling.
- **12 real executable workflows** through the actual input loop, each with
  isolated settings and a second startup to verify persistence: fresh defaults,
  physical volume editing/reset, tempo typing/cancellation/validation, absolute
  chord roots/octaves, front minor scale, independent envelopes, picker cancel,
  Light theme and clean/long-Esc exit. Captures are exactly **320×170**; the
  changed controls were visually inspected in both themes.
- All six suites also pass with **AddressSanitizer + UndefinedBehaviorSanitizer**.
  This macOS runtime does not support LeakSanitizer, so leak detection is excluded.

## Design observations and limits

- **An envelope shapes a sound; it cannot extend a non-looping sample.** A short
  plucked sample still ends naturally, even with high Sustain or a slow Attack.
  This is why slow-attack playback is tested with a sustained Organ sound.
- **Chord/Arp note lengths follow their pattern gates.** A long Attack may soften
  or suppress a short rhythmic hit; a long Release overlaps later hits. These
  remain explicit sound choices, rather than silently changing the pattern.
- **Custom roots are absolute.** Key transposes only degrees following Key;
  explicit roots/octaves remain fixed. This matches the agreed design and is
  tested across preset switching, restores and restarts.
- These checks simulate Cardputer key events locally and verify rendered sample
  values. They do not establish physical keyboard rollover, perceived latency,
  speaker bass response or subjective timbre. Those still need playing/listening
  on the device; there is no claim that simulated input is a hardware test.

## Reproduce

From the project root, using the existing project-local toolchain:

```sh
.venv/bin/cmake --build build/desktop -j4
.venv/bin/ctest --test-dir build/desktop --output-on-failure
.venv/bin/python scripts/test_local_flows.py
```

The scenario coverage is in [scenario_tests.cpp](../tests/scenario_tests.cpp).
Real input-loop workflows are in [test_local_flows.py](../scripts/test_local_flows.py).
Captures and logs stay under the ignored build directory. No player settings are
used or overwritten by these local checks.
