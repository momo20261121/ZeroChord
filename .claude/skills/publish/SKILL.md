---
name: publish
description: Release ZeroChord by building and testing the Debian package, tagging the release, and publishing the package on GitHub when approved. Run only when the user asks to publish or release.
disable-model-invocation: true
---

# Publish ZeroChord

Run the numbered steps from the project root. Invoking `/publish` authorizes
release preparation, commits on `dev`, a local merge into `main`, and a local tag.
Use the publication choice at step 4 for any GitHub action. Store submission
requires separate authorization. Preserve Git history.

**The `dev` branch is local-only. Never push it** (no `git push --all`, `--mirror`,
or `origin dev`). Only `main` and `v<version>` may ever be pushed, and only with
explicit approval. `scripts/install_git_guard.sh` installs a pre-push hook
that refuses `dev`; the preflight runs it.

**Follow the workspace guide** for how to run Git, GitHub, and store commands.

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
- Use the version if the user already specified it. Otherwise ask them to
  confirm or override the proposed version before continuing.
- Write it to `VERSION.txt`, then `grep -rn "<old version>"` over tracked files and fix any
  other mention (docs "currently X", CHANGELOG heading) except history.
- Update the changelog draft, README, user guides, contributor instructions,
  `.env.example`, notices, and the app's About and footer text where behavior
  changed, before building the candidate. Include small changes to controls and
  behavior. Review `store/LISTING.md` and the workspace's private store copy
  (`private/store/zerochord/`) together. Search tracked files for stale version
  references, renamed features, and old paths, keeping historical entries intact.

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

**Update the tests before running them.** Compare the release changes since the
last tag (commits and working changes) with the local suites, keyboard scripts
and fixtures, `scripts/test_local_flows.py`, and the installed-device acceptance
test `scripts/test_installed.py`. Update affected key sequences, labels and
expected results to match the intended behavior. Add coverage when the existing
tests miss a meaningful regression risk. Then run each check once.

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
5. Run `sh scripts/install_release_device.sh install`. With the Cardputer
   workspace SSH profile, it uses the existing `cardputer-sudo` helper and its
   ignored local credential file. For other targets it uses the configured
   SSH connection and the device's sudo policy. If a password prompt is needed,
   ask the user to run `sh scripts/install_release_device.sh install-interactive`
   with `.env` loaded, then wait for installation to finish.
6. `sh scripts/install_release_device.sh test` runs `scripts/test_installed.py <version>`
   on the device (installed version, `dpkg --verify`, launcher entry, evdev input, audio,
   framebuffer, persistence, clean exit; it restores the launcher) and fetches
   `build/installed-acceptance/result.json`.
7. If the UI changed since the last release, refresh store screenshots with
   `scripts/release_screenshots.py` and inspect them at native 320x170.

## 4. Finalize the documentation and package

- **Fix** only bugs that are small, clearly understood, local, and do not change designed
  behavior, settings formats or presets. After a fix, rerun the affected tests; if shipped
  code changed, redo steps 3.2-3.6 so the tested binary is the packaged one.
- **Report, do not fix**, anything larger, ambiguous, or a design question.
- Date the release section in `CHANGELOG.md` and verify the README, guides, store
  copy, and screenshots against the tested build. Finish these edits before
  approval, including corrections to wording and links.
- Record actual test results and manual checks in `docs/TESTING.md`. Prepare
  `PUBLISH.md` with the package details, launcher notes, and validation checklist.
- If bundled docs changed after installation, keep a scratch copy of the tested
  archive, then repackage with `scripts/package.py` and the tested binary.
  Extract both archives with `dpkg-deb -x` and compare files and modes outside
  `usr/share/doc/zerochord/`; compare control fields with `dpkg-deb -f` (a
  docs-only change may change `Installed-Size`). Confirm the extracted executable
  matches the SHA-256 from step 3.2. Reinstall and retest a candidate whose
  payload outside the bundled docs, or other control fields, changed.
- If only artwork changed after the test (launcher or store icon) and the
  executable's SHA-256 still matches, repackage and reinstall without rerunning
  the installed test. Check `dpkg --verify` and the installed icon's SHA-256 on
  the device. Say in `docs/TESTING.md` and `PUBLISH.md` that the test ran on the
  earlier package with the same executable.
- Record the final size and SHA-256 in `PUBLISH.md`, then run
  `scripts/privacy_check.py --deb` on the final archive and verify its checksum
  file. The package and docs presented for approval are the ones that will be
  tagged and published.

### Ready to release, publish, and submit

After the last repackaging, check against the final archive:

- `PUBLISH.md`: size, SHA-256 and executable SHA-256 match the archive, the
  `.sha256` file and the extracted executable. The validation list says which
  package each check ran on.
- `docs/TESTING.md`: the version's section records every run, including any
  repackaging after the test and why the test was not repeated.
- `CHANGELOG.md` and `README.md` match their copies in the archive
  (`usr/share/doc/zerochord/`); the version's section covers every visible change.
- `store/LISTING.md` and `private/store/zerochord/`: version, description, icon
  and screenshots match the final assets.
- Search tracked docs for old values of anything that changed (sizes, checksums,
  names). Leave earlier versions' sections as they are.

### Commit the candidate on dev and stage it on the device

- Commit the finished release files on local `dev` as `Release ZeroChord <version>`
  with the workspace identity from `.env` (the workspace GitHub account and its noreply address).
  Check `git log -1 --format='%an <%ae> | %cn <%ce>'` and that the worktree is
  clean. Record the commit with the final package checksum.
- Verify the final archive against the checksum in the committed `PUBLISH.md`,
  then `sh scripts/install_release_device.sh push`. Confirm the installed build is
  the final candidate; if the device has a different package, install the final
  archive and apply the retest rules above.
- If feedback changes the candidate, rerun affected checks, commit on `dev`, and
  repeat the device push and approval with the new commit and checksum.

## 5. Report and wait for approval

- Report concisely: version proposed/chosen; the candidate `dev` commit; bugs found and fixed (what, why, which test);
  unfixed problems with severity; checks actually run and results (privacy summary, test
  counts, installed-app result, final package size/SHA-256); what was *not* validated (physical
  keyboard feel, speaker listening) - never claim hands-on validation that did not happen;
  privacy WARNs needing confirmation.
- Ask for release approval and a publication choice. **Wait for approval.**
  - **Local release only (default).** Merge the approved commit and tag locally.
  - **Publish to GitHub.** Push `main` and `v<version>`, then create the GitHub
    Release with the final `.deb`, checksum, and release notes.
  - **Push refs only.** Push `main` and `v<version>` when the user requests a
    source-only push.
- A later request to publish the package on GitHub can resume step 7 from an
  existing release tag. Use the final package and checksum for that tag.

## 6. Create the local release

1. Final `python3 -I scripts/privacy_check.py --deb dist/...`; no FAIL.
2. Confirm the worktree is clean and `dev` still points to the approved candidate
   commit. Load the workspace identity from `.env` before merging and tagging. No
   extra branches; never amend or rewrite published history.
3. `git switch main && git merge --no-ff dev -m "Release ZeroChord <version>"`, then
   `git tag -a v<version> -m "ZeroChord <version>"` on `main`. Verify
   `git show main:VERSION.txt` and `git diff dev main --stat` is empty. Verify the
   tag includes the final docs and that the archive matches the recorded checksum.

## 7. Publish to GitHub

Check that the release tag contains the finalized documentation before the
first push.

1. For an approved GitHub publication or refs-only push, run
   `git push origin main v<version>`. Verify the remote tag matches the local
   annotated tag and confirm `git ls-remote --heads origin` shows no `dev`.
   When resuming from an existing tag, skip the push if the tag matches and
   the remote `main` already contains the tagged commit.
2. For an approved GitHub Release, use the account named by
   `ZEROCHORD_GH_USER` in `.env`. Pass the token to each `gh` command and keep
   the global `gh` account unchanged. Check that `gh api user --jq .login`
   returns the configured account. If it needs a login, ask the user to run
   `gh auth login` for that account.
3. Check the final package against its SHA-256 file. Save the version's section
   from the tagged `CHANGELOG.md` to a temporary notes file. If the release
   already exists, verify its assets before retrying any upload. Otherwise create it:

   ```sh
   (
     test -n "$ZEROCHORD_GH_USER" || exit 1
     zerochord_gh_token=$(gh auth token --user "$ZEROCHORD_GH_USER") || exit 1
     test -n "$zerochord_gh_token" || exit 1
     export GH_TOKEN="$zerochord_gh_token"
     test "$(gh api user --jq .login)" = "$ZEROCHORD_GH_USER" || exit 1
     gh release create "v$zerochord_version" \
       "dist/zerochord_${zerochord_version}_arm64.deb" \
       "dist/zerochord_${zerochord_version}_arm64.deb.sha256" \
       --repo "$zerochord_repo" --title "ZeroChord $zerochord_version" \
       --notes-file "$zerochord_notes_file" --verify-tag
   )
   ```

   Set `zerochord_version` to the approved version, `zerochord_repo` to the
   GitHub owner/repo from `origin`, and `zerochord_notes_file` to the notes path.
4. Use `gh release view` to confirm the release URL, published status, and both
   assets. Download the `.deb` and checksum to a scratch directory, then verify
   the downloaded package SHA-256 against the final local package. Check the
   public commits, tag, release author, asset uploader and release text against
   the workspace privacy rules before reporting it done.
5. Report the tag, package path and SHA-256, and the GitHub Release URL when
   published. For a local-only release, include `git push origin main v<version>`
   as the refs-only command. Complete any later documentation edits through a
   separate change. Store submission follows the workspace guide when authorized.

## 8. Start the next development cycle

- Return to `dev` and run `git merge --ff-only main` after the local release or
  approved GitHub publication. Ask for the next planned version using the rules
  in step 1 before bumping `VERSION.txt`. Then add a
  `## <next> (in development)` CHANGELOG heading, update the current versions
  in `AGENTS.md` and `docs/DEVELOPMENT.md`, and commit
  `Start <next> development` on local `dev`.

## Store submission

On a separate request, follow the workspace store steps with the final checked
package and listing. Record the pull request in `PUBLISH.md`.

## If something fails

Stop at the failing step, report the real output, and do not skip ahead. The installed-app
test restores the launcher in `finally`; if the display looks stuck, run
`ssh ... systemctl --user start APPLaunch.service`. To resume the dev preview afterwards use
`scripts/deploy_device.sh`.
