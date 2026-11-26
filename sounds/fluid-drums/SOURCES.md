# FluidR3 drum kits

Optional named drum kits, not replacements for the built-in sounds. Run
`python scripts/fetch_sound_packs.py` from the repository root, then copy this
whole `fluid-drums` folder into a [sound pack location](../README.md).
The download is kept project-local and is not committed to Git.

- Kits (bank 128): Standard (program 0), Room (8), Power (16), Electronic (24),
  TR-808 (25), Jazz (32) and Brush (40), from FluidR3 GM. The Orchestra kit (48)
  is left out: all eight kits came to 16,837,216 bytes, over the 16 MiB pack limit.
- Source: Debian package [`fluid-soundfont-gm 3.1-5.3`](https://deb.debian.org/debian/pool/main/f/fluid-soundfont/),
  `fluid-soundfont-gm_3.1-5.3_all.deb`, SHA-256
  `6f531493ac4e4d9772fd96b2488ea1790af81c196135fbdd25997da0781fc60e`; its
  `FluidR3_GM.sf2` has SHA-256
  `74594e8f4250680adf590507a306655a299935343583256f3b722c48a1bc1cb0`.
- File: `bank.sf2`, 14,691,522 bytes.
- SHA-256: `1b206d2f97e1eb58c50c4f48df2b49be9ab9d35b0e2f4a0cf01071e4b151c6df`
- License: MIT; see [LICENSE.txt](LICENSE.txt), copied from the package's
  `usr/share/doc/fluid-soundfont-gm/copyright`.
- Extraction: subset by `scripts/fetch_sound_packs.py` (`scripts/sf2_subset.py`)
  without sample processing. It keeps the seven kits, their 19 instruments and
  165 samples byte for byte, and rewrites only indices and the bank name and comment.

Samples are 44.1 kHz. FluidR3 marks its stereo samples left/right but links every
one to its first sample, "Gun"; the subset keeps that unused 30 KB sample so the
links stay as published. The 14.0 MiB pack fits the 16 MiB per-pack limit. With
`fluid-grand` and `fluid-synths`, the three packs total 22.5 MiB of the 32 MiB
allowance. It is not bundled in the installed package.
