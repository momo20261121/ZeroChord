# Application Store Publish Record

Keep this file in the application project root. Update it when package
metadata, listing copy, assets, privacy choices, or submission status changes.

Portal: <https://dev.cardputer.cc/#/upload>

## Package

- Debian package path: `dist/zerochord_0.1.0_arm64.deb` (native AArch64 build; verified)
- Package: `zerochord` (store ownership/availability not checked)
- Version: `0.1.0`
- Architecture: `arm64`
- Maintainer: `ZeroChord contributors <zerochord@example.invalid>` — development placeholder; replace before publishing
- APPLaunch desktop path: `/usr/share/APPLaunch/applications/zerochord.desktop`
- Executable path: `/usr/share/APPLaunch/bin/M5CardputerZero-ZeroChord`
- Wrapper: `/usr/share/APPLaunch/bin/run-zerochord`
- Runtime sound bank: Debian `timgm6mb-soundfont` dependency; not bundled
- Verified package SHA-256: `0b3e6255fbbee241dc00ba21a5ff1e69febea58b8c73a571d3747e675bf9ad74`
- Native device tests passed; launcher installation still awaits interactive sudo authentication.
- Includes shared/per-degree chord customization (no3/sus2/sus4), scope shortcuts,
  transactional choice lists with sound auditions, and confirmed mapping resets.
  Desktop/native/sanitizer suites and on-device evdev save/reload checks passed.
- O cycles Pad/Chord/Arp. Chord notes strike simultaneously with a falling
  envelope; style patterns, pad motion, and independent sounds are configurable.
- Nine TOML style presets include BPM, mode/patterns, sounds, levels, effects,
  and sweep timing. App settings own key/scale, Hold, and chord mappings. Individual defaults, remembered
  edits, save-as, master volume, and dark/light theme are included. Settings use
  Style/App tabs with Left/Right navigation and Enter editors.
- Display recovery verified: preview wakes a sleeping backlight; kernel framebuffer
  readback matches the rendered UI within RGB565 quantization error. Physical panel
  visibility still needs confirmation on the unit.

## Source and Ownership

- Public source repository: TODO — local project only
- Expected GitHub uploader/owner: TODO
- Hide uploader email: `true` (template default; confirm before submission)
- Source license: MIT; dependency/font notices included separately

## Listing

- Application name: ZeroChord
- One-line summary (draft, maximum 80 characters): A pocket chord-and-harp instrument for CardputerZero.
- Description (draft): Play scale-relative chords, pick harp notes, and strum from the CardputerZero keyboard. Customize chord combinations for all degrees or individual degrees, including no3, sus2, and sus4. Inspired by minichord and Omnichord.
- Categories: TODO — candidates: Creative & Office, Media

## Assets

- 256 x 256 source icon: `assets/share/images/zerochord.png` — replaceable generated music icon
- Icon packaged in `.deb`: `/usr/share/APPLaunch/share/images/zerochord.png`
- Screenshot 1 (320 x 170): TODO
- Screenshot 2 (320 x 170): TODO
- Screenshot 3 (320 x 170): TODO
- Screenshot 4 (320 x 170): TODO
- Optional screenshot 5 (portal supports up to 6): not planned
- Optional screenshot 6 (portal supports up to 6): not planned

## Preflight

- [x] `.deb` exists and can be parsed.
- [x] Control fields contain Package, Version, Architecture, and Maintainer.
- [x] Architecture is appropriate for the package (`arm64` for native binaries).
- [x] APPLaunch `.desktop`, executable, and PNG icon exist inside the package.
- [ ] Package name ownership and update version are valid.
- [ ] Listing copy and categories have been reviewed.
- [ ] Four clean 320 x 170 baseline screenshots exist.
- [x] No credentials, tokens, or private server details are embedded.
- [ ] The user explicitly approved external submission.

## Submission Result

- Status: `not submitted` — local prototype; hardware acceptance and public maintainer/source details pending
- Submitted at: TODO
- Portal message: TODO
- Actions URL: TODO
- Tracking or pull-request URL: TODO
