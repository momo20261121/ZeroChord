# Third-party notices

ZeroChord's original source is MIT licensed. Dependency licenses are retained
in [third_party/licenses](third_party/licenses).

| Component | Version | License | Use |
| --- | --- | --- | --- |
| [LVGL](https://github.com/lvgl/lvgl) | 9.5.0 | MIT | Native 320 x 170 interface |
| [SDL](https://github.com/libsdl-org/SDL) | 2.32.10 | zlib | Desktop preview only |
| [TinySoundFont](https://github.com/schellingb/TinySoundFont) | 853a0a171759f1ddba0de1442133a75912bbeffa | MIT | SoundFont renderer |
| [toml++](https://github.com/marzer/tomlplusplus) | 3.4.0 | MIT | Preset configuration parser |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.25 | MIT-0 option | Audio output |
| [Montserrat](https://github.com/JulietaUla/Montserrat) | LVGL bundled glyphs | SIL OFL 1.1 | Display font |
| [Unscii 8](http://viznut.fi/unscii/) | LVGL bundled glyphs | Public domain | Key legends and Roman numerals |
| [Font Awesome 5](https://github.com/FortAwesome/Font-Awesome/tree/5.x) | LVGL bundled glyphs | SIL OFL 1.1 (font) | Symbols embedded in LVGL fonts |
| [minichord firmware](https://github.com/BenjaminPoilve/minichord) | Reference, not linked | BSD-3-Clause | Combination chord and note-lifetime design reference |
| [CardputerZero Piano](https://github.com/CardputerZero/Piano) | Reference, not linked | MIT | Audio/backend architecture reference |

ZeroChord implements its own music state machine and platform layer. It does
not include minichord's Teensy firmware or Omnichord assets. Minichord firmware
copyright: 2024 Benjamin Poilvé. Piano source copyright: 2026 M5Stack CardputerZero Community.
Reference licenses are retained conservatively for future adaptations.

The optional Grand Piano (FluidR3) pack uses the piano SoundFont from the
CardputerZero Piano repository. The download script verifies the pinned asset's
SHA-256. The upstream repository records the SoundFont's source and license.
[Pinned SoundFont source](https://github.com/CardputerZero/Piano/tree/703eb1e0926c9d4ab3b34997e854c06b180e2c7b/assets/soundfonts)

The optional **fluid-synths** and **fluid-drums** packs are subsets of
FluidR3 GM, built by `scripts/fetch_sound_packs.py` from Debian's
`fluid-soundfont-gm 3.1-5.3` package (pinned SHA-256). FluidR3 GM is MIT
licensed, copyright 2000-2002, 2008 Frank Wen and 2008 Toby Smithe. Each pack
folder keeps the license text, copied from the package's `copyright` file, in
`LICENSE.txt` and its provenance in `SOURCES.md`
([fluid-synths](sounds/fluid-synths/SOURCES.md),
[fluid-drums](sounds/fluid-drums/SOURCES.md)). Neither pack is bundled in the
installed package.

Font Awesome Free font glyphs: copyright Fonticons, Inc.
The full SIL OFL 1.1 text is included in `Montserrat-OFL.txt`, alongside the
Font Awesome project's license/attribution notice in `FontAwesome5.txt`.

## Runtime sound bank

The device package depends on Debian's `timgm6mb-soundfont`; it **does not bundle**
the bank. TimGM6mb is GPL-2-only, copyright Tim Brechbill (2004) and David Bolton
(2010). Its copyright and license are supplied by that package under
`/usr/share/doc/timgm6mb-soundfont/copyright` and
`/usr/share/common-licenses/GPL-2`.

The ignored local development copy is copied from the device for private tests.
Do not redistribute it as an MIT asset. Any future bundled bank needs its own
provenance, license notices, and distribution obligations reviewed.

Build tools under `.tools/` and `.venv/` are development dependencies and are
excluded from the device package.
