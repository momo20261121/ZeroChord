# FluidR3 synths

Optional named synth sounds, not replacements for the built-in sounds. Run
`python scripts/fetch_sound_packs.py` from the repository root, then copy this
whole `fluid-synths` folder into a [sound pack location](../README.md).
The download is kept project-local and is not committed to Git.

- Sounds (bank 0): Square Lead (program 80), Saw Lead (81, FluidR3 "Saw Wave"),
  Synth Bass (38, "Synth Bass 1"), Synth Brass (62, "Synth Brass 1"),
  Polysynth (90) and Synth Strings (50, "Synth Strings 1"), from FluidR3 GM.
- Source: Debian package [`fluid-soundfont-gm 3.1-5.3`](https://deb.debian.org/debian/pool/main/f/fluid-soundfont/),
  `fluid-soundfont-gm_3.1-5.3_all.deb`, SHA-256
  `6f531493ac4e4d9772fd96b2488ea1790af81c196135fbdd25997da0781fc60e`; its
  `FluidR3_GM.sf2` has SHA-256
  `74594e8f4250680adf590507a306655a299935343583256f3b722c48a1bc1cb0`.
- File: `bank.sf2`, 1,065,558 bytes.
- SHA-256: `255f3f92fbc2533df8db9bdf18e6750b86b119f066b54cf5eb0b37af4b92ec0c`
- License: MIT; see [LICENSE.txt](LICENSE.txt), copied from the package's
  `usr/share/doc/fluid-soundfont-gm/copyright`.
- Extraction: subset by `scripts/fetch_sound_packs.py` (`scripts/sf2_subset.py`)
  without sample processing. It keeps the six presets, their 7 instruments and
  31 samples byte for byte, and rewrites only indices and the bank name and comment.

Samples are 44.1 kHz, except Synth Brass (25 kHz). FluidR3 marks its stereo
samples left/right but links every one to its first sample, "Gun"; the subset keeps
that unused 30 KB sample so the links stay as published. The 1.0 MiB pack fits
the per-pack limit; it is not bundled in the installed package.
