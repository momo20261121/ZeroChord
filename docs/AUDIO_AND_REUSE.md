# Audio sourcing and source reuse

Reviewed online: 2026-11-21. No upstream source or sound assets have been copied
into ZeroChord. This records candidate components and the license terms inspected;
it is not a completed audit of every upstream dependency.

## Product direction

Play like minichord, with the agreed CardputerZero keyboard adaptation. Use
openly licensed sound sources; exact timbral emulation is not a requirement.
Keep musical behavior independent from the sound renderer so the same chords,
harp notes, strums, and patterns work with different instruments.

### Sound quality observations (2026-11-22)

The current TimGM6mb development bank is 5,969,788 bytes. Inspecting its actual
sample headers gives Strings at 12 kHz, Piano and Warm Pad at 22.05 kHz, Music
Box at 22,321 Hz, and Airy Pad at mixed 22.05/44.1 kHz. The three newly exposed
guitars and Celesta use 44.1 kHz samples. Source rate alone does not establish
perceived quality, but the lower rates limit available high-frequency detail.

The app renders 48 kHz stereo floats; TinySoundFont uses linear sample
interpolation. A higher output rate cannot recover missing recorded detail.
Prioritize auditioning better licensed multisamples for Piano, Strings, and
Music Box. Compare the same offline render on computer headphones and the
Cardputer speaker before attributing the whole difference to the engine.
Keep sound-bank changes separate from engine/effect changes for a useful A/B
comparison. Imported bank memory limits are documented in [custom sounds](../sounds/README.md).

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
for additional instruments, and a pinned, MIT-licensed Piano extraction is now available as an optional
comparison pack. See [its provenance](../sounds/fluid-grand/SOURCES.md).

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

## Playback articulation

The TimGM6mb Warm Pad's 606 ms attack and 2.57 s release can hide short triggers.
Chord and Arp now cap the native voice attack at 10 ms and release at 200 ms
(250 ms for sustained Chord), preserving native decay/sustain and note-dependent
timing. Explicit per-part ADSR settings override these bounds; Pad uses its own
unadapted envelope. No shared SoundFont regions are changed.

Regular eighth-note arps use a 1.9-sixteenth gate for each two-sixteenth interval
(95%), avoiding the former 40% gate gap. Deliberately syncopated patterns keep
rests. Independent Chord and Arp schedulers share one transport; Pad motion
modulates only Pad channels. Combined backing layers use 1/√(layer count) gain
for headroom. Front remains independent.

The adapter uses TinySoundFont internals at the pinned revision above; recheck
its envelope setup when updating that dependency. Local tests cover finite,
audible rendering, no clipping at normal settings, part/envelope routing, all
eight layer combinations, flowing-arp continuity, gate release and panic.
Listening on the device still determines whether the musical starting points
feel right; numerical checks do not establish subjective sound quality.


## Source sample rates and perceived quality

Sound pickers read the loaded preset's sample-region rates. The current TimGM6mb
bank uses 22.05 kHz Piano, 12 kHz Strings, 22.321 kHz Music Box, 44.1 kHz Celesta,
and mixed 22.05–44.1 kHz Airy Pad samples. Imported banks are inspected the same
way. The engine renders at 48 kHz; resampling cannot recover missing source
detail. A displayed rate describes the source, not its recording quality.

A synthetic or “plastic” character can also come from sparse sample zones,
looping, fixed velocity, envelopes, and playback through the small speaker.
These are likely contributors, not a listening diagnosis. TinySoundFont uses
linear interpolation; changing interpolation may help transposed samples but
does not replace a better recording. The practical next comparison is a clearly
licensed, more detailed Piano bank, then Strings, using identical notes, envelopes
and matched volume. Compare headphones or recorded output as well as the speaker.
Keep deliberately rough sounds named as such; do not label every 44.1 kHz sound
Hi-fi. No new bank or interpolation change is included in this metadata update.


## Controlled piano and renderer comparison

The optional Grand Piano (FluidR3) pack retains its upstream 32 kHz samples. It
is a separate named sound; built-in Piano and factory preset defaults remain
available. This is a listening trial, not a claim of improved sound quality.

`zerochord-compare` renders the same phrase with built-in Piano, FluidR3 Piano,
and Electric Piano with Echo off/on. Native sample envelopes are preserved, EQ
is flat, and the WAV files are RMS matched using attenuation only. A combined
file plays them in that order with half-second separators.

The tool also compares native TinySoundFont against ZeroChord's dry Front path
for Piano, Electric Piano, Harp and Warm Pad, using aligned blocks and the same
velocity, gain, notes and releases. They currently match sample-for-sample. This
rules out extra coloration in that dry wrapper for those cases; it does not rule
out limitations in TinySoundFont, the bank, explicit envelopes/effects or the
analog output. The tool rejects mismatches, silence, nonfinite values and clipping.

Device inspection found ES8389 playback at 48 kHz, stereo S32_LE, digital DAC
gains at 0 dB, no output polarity inversion and no microphone loopback. S32_LE
describes the PCM container, not measured DAC resolution or analog fidelity.
Compare the same WAV through another device, Cardputer earphones, and its
speaker at moderate matched loudness. A PipeWire monitor cannot measure the
DAC, amplifier or speaker. No speculative DAC or interpolation change is made.


### Independent engine check

A subsequent offline comparison used macOS AVAudioUnitSampler as a separate
SF2 renderer, loading exactly the same TimGM Electric Piano and FluidR3 Piano
files. Each pair uses the same phrase, note starts/releases and MIDI velocity
114. No additional reverb, chorus, EQ or echo nodes are used; CC91/93 are zero.
The four 9-second clips are RMS matched by attenuation only, with half-second
separators. Neither the native TSF comparison nor this engine comparison measures
analog DAC/amp quality. Apple Sampler is a diagnostic reference, not a Linux
backend candidate or a guarantee of complete SoundFont compliance.

TinySoundFont's dry wrapper matching itself does not validate TinySoundFont's
SF2 interpretation against another renderer. The pinned TSF source lists missing
modulator support and limitations in its low-pass filter; it uses linear sample
interpolation. These are plausible timbre differences, not a confirmed cause of
the reported coloration. Front notes also use fixed velocity near 114/127; volume
changes amplitude rather than strike velocity, which can retain a hard/bright
character. No renderer or velocity default has been changed on this evidence.

References: [Apple SF2 loading](https://developer.apple.com/library/archive/technotes/tn2283/_index.html),
[pinned TinySoundFont source](https://github.com/schellingb/TinySoundFont/blob/853a0a171759f1ddba0de1442133a75912bbeffa/tsf.h).
