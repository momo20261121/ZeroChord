---
name: publish
description: Release ZeroChord - choose the version, privacy-check, build/package/install/test the .deb on the Cardputer, report, update docs, and merge dev into main. Run only when the user asks to publish or release.
disable-model-invocation: true
---

# Publish ZeroChord

Run the six steps in order from the project root. Invoking `/publish` authorizes the
routine release work below (commits on `dev`, local merge into `main`, local tag).
It does **not** authorize pushing, store submission, or rewriting history.
Two hard stops need the user: the version choice (step 1) and the report (step 4).

**The `dev` branch is local-only. Never push it** (no `git push --all`, `--mirror`,
or `origin dev`). Only `main` and `v<version>` may ever be pushed, and only after the
user approves that at step 4. `scripts/install_git_guard.sh` installs a pre-push hook
that refuses `dev`; the preflight runs it.

Load private configuration in every shell that needs it: `set -a; . ./.env; set +a`.
Use project-local tools only (`.venv`, `.tools`); install nothing system-wide.

## 0. Preflight

- `git branch --show-current` must be `dev`; switch to it if on `main`. Note `git status`.
- `sh scripts/install_git_guard.sh`. Run `git ls-remote --heads origin dev`; if a remote
  `dev` exists, report it (do not delete it).
- Device reachable: `ssh` to `$ZEROCHORD_SSH_HOST` works (see `scripts/deploy_device.sh`
  for how options are built). If not, stop and say so.

## 1. Decide the version

- Last release: newest `v*` tag. Review `git log <tag>..HEAD --oneline`, `git diff <tag> --stat`
  and the CHANGELOG "in development" section.
- Propose a semantic version: **patch** for fixes, tuning and small features that keep
  saved settings and presets compatible; **minor** for significant new player-facing
  features; **major** for incompatible settings/preset/control changes. Give a
  one-sentence reason.
- Ask the user to confirm or override (AskUserQuestion). **Wait for the answer.**
- Write it to `VERSION.txt`, then `grep -rn "<old version>"` over tracked files and fix any
  other mention (docs "currently X", CHANGELOG heading) except history.

## 2. Privacy check

- `python3 -I scripts/privacy_check.py` scans every tracked and untracked-unignored file
  and commit metadata since the last tag for private values (device host, SSH config,
  account, home-directory and project locations, git emails, `ZEROCHORD_PRIVATE_TERMS`), keys/tokens and
  stray emails/paths.
- Any **FAIL**: remove the value from the file or commit data and move it to `.env`, then
  rerun until clean. Commit history is never rewritten; if history is affected, report it.
- **WARN**: judge each. The public project URL / maintainer contact may appear where
  intended (e.g. the About QR in `src/ui.cpp`); list those in the report for the user's
  explicit confirmation. Check `git status` for files that should not ship.
- Record the one-line summary the script prints for the report.

## 3. Build, package, install, test

1. Local checks (once, before building): desktop build, `.venv/bin/ctest --test-dir build/desktop
   --output-on-failure`, and `.venv/bin/python scripts/test_local_flows.py`. Add the sanitizer
   build (see `docs/TESTING.md`) if audio, input or lifecycle code changed since the last tag.
2. `.venv/bin/python scripts/build_device.py` then `.venv/bin/python scripts/package.py`
   (needs `ZEROCHORD_MAINTAINER`). Record `shasum -a 256 build/cross-device/M5CardputerZero-ZeroChord`
   and the `.deb` size/SHA-256.
3. `python3 -I scripts/privacy_check.py --deb dist/zerochord_<version>_arm64.deb` (also verifies
   the package version and maintainer). Must have no FAIL.
4. `sh scripts/install_release_device.sh push` copies the package and test script and verifies
   the checksum on the device.
5. Install needs the device's sudo password, which Claude cannot type. First try
   `ssh ... sudo -n true`; if it fails, ask the user to run:
   `! set -a; . ./.env; set +a; sh scripts/install_release_device.sh install`
   and wait for them to say it finished. Never store or pass a password.
6. `sh scripts/install_release_device.sh test` runs `scripts/test_installed.py <version>`
   on the device (installed version, `dpkg --verify`, launcher entry, evdev input, audio,
   framebuffer, persistence, clean exit; it restores the launcher) and fetches
   `build/installed-acceptance/result.json`.
7. If the UI changed since the last release, refresh store screenshots with
   `scripts/release_screenshots.py` and inspect them at native 320x170.

## 4. Report and wait for approval

- **Fix** only bugs that are small, clearly understood, local, and do not change designed
  behavior, settings formats or presets. After a fix, rerun the affected tests; if shipped
  code changed, redo steps 3.2-3.6 so the tested binary is the packaged one.
- **Report, do not fix**, anything larger, ambiguous, or a design question.
- Report concisely: version proposed/chosen; bugs found and fixed (what, why, which test);
  unfixed problems with severity; checks actually run and results (privacy summary, test
  counts, installed-app result, package size/SHA-256); what was *not* validated (physical
  keyboard feel, speaker listening) - never claim hands-on validation that did not happen;
  privacy WARNs needing confirmation.
- **Stop.** Ask (AskUserQuestion): approve release / fix more / abort, and separately whether
  to push `main` + `v<version>` to origin (default: do not push). Proceed only on approval.

## 5. Update docs

- `CHANGELOG.md`: replace "(in development)" with `## <version> - <YYYY-MM-DD>`; list
  user-facing changes in plain language, including fixes from step 4; no private data.
- `README.md`, `docs/USER_GUIDE.md`, other docs: only where behavior or controls changed.
- `docs/TESTING.md`: add a section with the real results and the limits from step 4.
- `PUBLISH.md`: rewrite the release record for this version (package table with size and
  SHA-256, validation checklist with unchecked manual items, launcher/install notes).
- Docs are packaged inside the `.deb`, so rerun `scripts/package.py` afterwards and confirm
  the binary hash equals the one recorded in step 3.2 (no rebuild in between), then rerun
  `privacy_check.py --deb`. Update the SHA-256 in PUBLISH.md to the final file. Say that the
  final package differs from the tested one only in bundled docs.

## 6. Release to main

1. Final `python3 -I scripts/privacy_check.py --deb dist/...`; no FAIL.
2. Commit everything on `dev` as one release commit with the project contributor identity
   (`set -a; . ./.env; set +a` first; check `git log -1 --format='%an <%ae>'`):
   `Release ZeroChord <version>`. No extra branches; never amend or rewrite published history.
3. `git switch main && git merge --no-ff dev -m "Release ZeroChord <version>"`, then
   `git tag -a v<version> -m "ZeroChord <version>"` on `main`. Verify
   `git show main:VERSION.txt` and `git diff dev main --stat` is empty.
4. Back on `dev`: `git merge --ff-only main`, then start the next cycle: bump `VERSION.txt`
   to the next planned version (propose it by the step 1 rules and ask the user; do not assume a patch), add a `## <next> (in development)` CHANGELOG heading, update the
   "currently X" mentions in `AGENTS.md` and `docs/DEVELOPMENT.md`, commit
   `Start <next> development`. `dev` stays local.
5. Push only if approved in step 4, naming refs explicitly:
   `git push origin main v<version>`. Then confirm `git ls-remote --heads origin` shows no
   `dev`. If not approved, print that exact command for the user.
6. GitHub Release (only if the user approved it at step 4): use this project's account without
   switching the global `gh` login, and never `gh auth switch`:
   `GH_TOKEN=$(gh auth token --user "$ZEROCHORD_GH_USER") gh release create v<version> dist/zerochord_<version>_arm64.deb dist/zerochord_<version>_arm64.deb.sha256 --title "ZeroChord <version>" --notes-file <changelog section> --verify-tag`.
   If that account is not logged in, ask the user to run `! gh auth login`. Also ask about this
   at step 4, next to the push question.
7. Final summary: version, commit/tag, package path and SHA-256, push state, release link. Store upload
   (`PUBLISH.md` has the listing text) is a separate manual step the user must request.

## If something fails

Stop at the failing step, report the real output, and do not skip ahead. The installed-app
test restores the launcher in `finally`; if the display looks stuck, run
`ssh ... systemctl --user start APPLaunch.service`. To resume the dev preview afterwards use
`scripts/deploy_device.sh`.
