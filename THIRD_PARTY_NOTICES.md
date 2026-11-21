# Third-party notices

ZeroChord's original source is MIT licensed. Dependency licenses are retained
in [third_party/licenses](third_party/licenses).

| Component | Version | License | Use |
| --- | --- | --- | --- |
| [LVGL](https://github.com/lvgl/lvgl) | 9.5.0 | MIT | Native 320 x 170 interface |
| [SDL](https://github.com/libsdl-org/SDL) | 2.32.10 | zlib | Desktop preview only |
| [TinySoundFont](https://github.com/schellingb/TinySoundFont) | 853a0a171759f1ddba0de1442133a75912bbeffa | MIT | SoundFont renderer |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.25 | MIT-0 option | Audio output |
| [minichord firmware](https://github.com/BenjaminPoilve/minichord) | Reference, not linked | BSD-3-Clause | Combination chord and note-lifetime design reference |
| [CardputerZero Piano](https://github.com/CardputerZero/Piano) | Reference, not linked | MIT | Audio/backend architecture reference |

ZeroChord implements its own music state machine and platform layer. It does
not include minichord's Teensy firmware or Omnichord assets. Minichord firmware
copyright: 2024 Benjamin Poilvé. Piano source copyright: 2026 @MrFatTiger.
Reference licenses are retained conservatively for future adaptations.

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
