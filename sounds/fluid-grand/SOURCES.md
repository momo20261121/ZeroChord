# FluidR3 grand piano

Optional named alternative, not a replacement for the built-in Piano. Run
`python scripts/fetch_piano_pack.py` from the repository root, then copy this
whole `fluid-grand` folder into a [sound pack location](../README.md).
The download is kept project-local and is not committed to Git.

- Sound: Yamaha Grand Piano, bank 0 / program 0, extracted from FluidR3 GM.
- Source: [CardputerZero/Piano](https://github.com/CardputerZero/Piano/tree/703eb1e0926c9d4ab3b34997e854c06b180e2c7b/assets/soundfonts)
- Revision: `703eb1e0926c9d4ab3b34997e854c06b180e2c7b`
- Upstream package: `fluid-soundfont-gm 3.1-5.3`.
- File: `piano.sf2`, 7,833,208 bytes.
- SHA-256: `addb732569676727b02777a8df1f5d49eca0a19d4f306cca31624fdbfa0e1909`
- License: MIT; see [LICENSE.txt](LICENSE.txt).
- Upstream extraction: exported as SFZ and rebuilt as a one-preset SF2 using
  Polyphone 2.2; no sample processing or resampling. ZeroChord leaves it unchanged.

Sample rates describe the source, not a quality score. Compare this variant at
matched volume with the built-in Piano before deciding which you prefer. The
7.5 MiB pack fits the existing per-pack limit; it is not bundled in the installed package.
