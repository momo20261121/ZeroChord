# Audio sourcing and source reuse

Reviewed online: 2026-11-21. No upstream source or sound assets have been copied
into ZeroChord. This records candidate components and the license terms inspected;
it is not a completed audit of every upstream dependency.

## Product direction

Play like minichord, with the agreed CardputerZero keyboard adaptation. Use
openly licensed sound sources; exact timbral emulation is not a requirement.
Keep musical behavior independent from the sound renderer so the same chords,
harp notes, strums, and patterns work with different instruments.

Recommended first path:

1. Adapt the relevant minichord musical rules to the scale-degree keyboard.
2. Follow CardputerZero Piano's approach for SF2 playback and audio output.
3. Audition and package a small selection of openly licensed sounds for the
   chord and harp parts, with separate program selection and volume.
4. Consider a port of selected minichord synthesis later if it adds useful sounds.

## Minichord

Primary references:

- [Repository and software/hardware license split](https://github.com/BenjaminPoilve/minichord)
- [Firmware license](https://github.com/BenjaminPoilve/minichord/blob/main/firmware/LICENSE)
- [Musical logic](https://github.com/BenjaminPoilve/minichord/blob/main/firmware/src/main.cpp)
- [Audio graph](https://github.com/BenjaminPoilve/minichord/blob/main/firmware/include/audio_definition.h)
- [Parameters and defaults](https://github.com/BenjaminPoilve/minichord/blob/main/firmware/generator/parameters.json)
- [Target configuration](https://github.com/BenjaminPoilve/minichord/blob/main/firmware/platformio.ini)

### What can be useful

The firmware includes chord interval tables, same-column combination selection,
cross-column slash detection, chord and harp note calculation, inversions and
spacing, rhythm scheduling, and configurable behavior for held strings.

Concrete candidates in the main firmware source are `handle_chord_type`,
`detect_slash`, `calculate_note_chord`, `calculate_note_harp`, and
`rythm_tick_function`. These are adaptation candidates, not standalone modules:
they use global state and some call platform-specific functions.

The parameter definitions are also useful for understanding which behaviors are
musical settings rather than fixed rules. For example, held strings can retain
their pitch or follow a changed chord, and chord retriggering is configurable.

### Software license

The firmware's general license is BSD-3-Clause, copyright Benjamin Poilve, 2024.
It permits redistribution and use of original or modified source and binaries,
including commercial use. For covered code:

- Retain the copyright notice, conditions, and disclaimer in source distributions.
- Include those notices in documentation or other materials distributed with binaries.
- Do not use the copyright holder's or contributors' names to imply endorsement
  without the required permission.

BSD-3-Clause does not require publishing ZeroChord's source or licensing all of
ZeroChord under BSD. Preserve the upstream license for any adapted portions.

The license explicitly says licenses in library folders override the general
license. Do not label all bundled libraries BSD merely because the main firmware
is BSD. Inspect the particular dependencies before copying or porting them.

### Hardware license

The repository identifies the hardware as CC BY-NC 4.0. That is separate from the
firmware's BSD terms. The current ZeroChord plan uses CardputerZero hardware and
does not call for copying minichord enclosure, PCB, or keycap manufacturing assets.

### Porting boundary

The project targets Arduino on Teensy 4.0. Its audio graph uses Teensy Audio
oscillators, envelopes, filters, mixers, and related processing. It is synthesis
code, not an SF2 bank that can simply be loaded by TinySoundFont.

The musical rules can be adapted to platform-independent C++; copying the whole
firmware will not produce a working Linux application. Recreating the synthesis
would require a compatible DSP implementation, adapting timing and audio routing,
and reviewing the licenses of the actual DSP dependencies selected.

The upstream program also assumes its own physical root ordering and twelve harp
zones. ZeroChord needs the agreed II–I order and seven ascending harp notes;
adapt those assumptions deliberately.

## CardputerZero Piano

Primary references:

- [Repository](https://github.com/CardputerZero/Piano)
- [Application MIT license](https://github.com/CardputerZero/Piano/blob/main/LICENSE)
- [Build and backend configuration](https://github.com/CardputerZero/Piano/blob/main/CMakeLists.txt)
- [Dependency versions](https://github.com/CardputerZero/Piano/blob/main/repos.json)
- [SoundFont renderer](https://github.com/CardputerZero/Piano/blob/main/src/audio/soundfont_renderer.cpp)
- [Third-party notices](https://github.com/CardputerZero/Piano/blob/main/THIRD_PARTY_NOTICES.md)
- [Sound-bank provenance](https://github.com/CardputerZero/Piano/blob/main/assets/soundfonts/SOURCES.txt)

Piano uses C++17, LVGL, TinySoundFont, and miniaudio. Its device build defaults to
the PulseAudio backend; its desktop development build supports SDL. This is a
relevant implementation reference, not a substitute for measuring ZeroChord's
input latency and polyphony on the device.

Piano's application code is MIT licensed. Its bundled bank contains only Yamaha
Grand Piano, extracted from FluidR3 GM. The provenance record describes the bank
as MIT licensed and records its original Debian package, extraction process,
attribution, and SHA-256 checksum.

It is a useful first reference sound, but it is not a multi-instrument bank.
Additional chord and harp sounds must be sourced separately. A held piano note
decays naturally; a continuous chord bed needs an appropriate sustaining/looped
instrument or a synthesis voice rather than an assumption that any sample lasts
indefinitely.

## Candidate rendering components

| Component | Purpose | License inspected |
| --- | --- | --- |
| TinySoundFont | Render SF2 instruments | MIT at Piano's pinned revision |
| miniaudio | Audio output and mixing infrastructure | Choice of public-domain dedication or MIT-0 at 0.11.25 |
| Piano's extracted FluidR3 piano | Initial sampled reference sound | MIT according to its provenance and accompanying notice |

Inspected dependency licenses:

- [TinySoundFont at Piano's pinned revision](https://github.com/schellingb/TinySoundFont/blob/853a0a171759f1ddba0de1442133a75912bbeffa/LICENSE)
- [miniaudio 0.11.25](https://github.com/mackron/miniaudio/blob/0.11.25/LICENSE)

The open-source renderer and the sound-bank assets are separate components.
Choose each sound asset with clear redistribution terms; a free download alone
does not establish permission to package it. FluidR3 GM is a candidate source
for additional instruments, but no additional presets have been selected,
downloaded, audited, or auditioned for ZeroChord yet.

## Preset structure and implementation notes

A ZeroChord preset should pair a chord instrument with a harp instrument, their
levels, register, and any chosen effects. Maintain independent note lifetimes
for chord, harp, and arpeggiator triggers, including overlapping occurrences of
the same pitch. Switching a preset must not leave stuck notes.

Keep sound-bank loading outside the real-time rendering callback and limit
polyphony deliberately. Small curated banks are preferable to shipping a full
General MIDI collection without a product need. Verify memory and CPU costs on
the actual device before fixing the voice count or effects budget.

When adopting code or assets, pin the exact source revision/version, preserve its
license and attribution, and record any modifications and asset checksums. ZeroChord's original application source is MIT licensed.

## Playback articulation in the prototype

The selected TimGM6mb Warm Pad has a 606 ms amplitude attack and 2.57 s release.
Those envelopes hide short arpeggio triggers, blending them into a steady pad.
Finite backing notes cap attack at 5 ms, decay over 220 ms to 8% sustain,
and cap release at 30 ms; sustained Pad
keeps the bank's original envelope. This changes each new voice's envelope,
without modifying shared SF2 regions or changing the selected instrument.
The adapter uses TinySoundFont internals at the pinned revision above; retest
its envelope setup when updating that dependency. Automated audio tests check
prompt attacks and quiet gaps for all four presets, plus the full Dream
Pad → Chord → Arp control path.
