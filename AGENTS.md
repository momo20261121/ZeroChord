# ZeroChord development

> **Read the [workspace guide](../../AGENTS.md) first.** Its rules apply to this project
> and take precedence over this guide, especially for Git, GitHub, and the store.

Prioritize short, playable iterations. The user's latest request defines the
scope; preserve previously agreed behavior unless the request changes it.

## Branches

- Keep `main` for tested releases; tag each release as `v<version>`.
- Use `dev` for development toward the next release (currently 2.0.1).
  Switch to it before changing application code while on `main`.
- Merge into `main` only when the release is tested and ready.
- `dev` is local-only: never push it or set it to track a remote. Only `main` and
  `v<version>` may be pushed, and only with authorization. `scripts/install_git_guard.sh`
  installs a pre-push hook that refuses `dev`.
- Do not rewrite existing release history or push without authorization.
- Release with the `/publish` workflow (`.claude/skills/publish/SKILL.md`).

## Privacy

- Keep device hosts, usernames, paths and credentials in ignored local
  environment files; track only placeholder examples.
- Read public package identity from `ZEROCHORD_MAINTAINER` and optional
  `ZEROCHORD_HOMEPAGE`, never from hard-coded personal values.
- Use the project contributor identity for commits unless explicitly requested
  otherwise. Preserve required third-party license attribution.

## Scope and stopping rule

- State a brief acceptance target, then implement it. Do not turn a small change
  into a redesign, cleanup, new framework, or extra feature.
- Batch the necessary code, settings migration, affected tests, and concise docs
  before building for the device. Avoid a build for each intermediate edit.
- After relevant local checks pass, freeze the feature scope. Only fix failures
  or problems that prevent the requested behavior from working.
- Finish when the requested behavior works, proportional checks pass, and the
  updated app is available to play when device deployment is in scope. Let the
  user's playing feedback guide further improvements.
- Do not introduce approval steps for routine, reversible work already covered
  by the request. Do not commit, publish, or install system dependencies unless
  authorized.

## Test locally first

- Use the existing project-local tools and dependencies. Prefer local installs
  when a dependency is necessary.
- Run the existing checks relevant to the change. Add regression coverage for
  meaningful state, migration, or playback risks; avoid tests that merely repeat
  the implementation.
- Check a layout locally at 320×170 before sending it to the device. Docs-only
  edits need no compilation, audio tests, or hardware run.
- After local checks pass, cross-compile in the Debian VM with
  `scripts/build_device.py`, then transfer the binary with
  `scripts/deploy_device.sh`. Keep the incremental build and compiler caches.
  Do not compile on the device during normal development. Build only the app
  target, without cross-building test executables unless needed.
- Do not routinely run local test suites a second time on the device. For a
  routine settings or menu change, passing local tests plus a successful device
  launch is sufficient. Add a focused hardware check only for a concrete risk
  that local tests cannot cover, such as framebuffer, physical input, audio
  backend, or launcher lifecycle behavior. Explain that risk briefly.
- Broaden testing for changes to audio rendering, input handling, lifecycle, or
  an observed regression. Do not routinely repeat every suite on every backend.

## Device turnaround

- The project-local Zig toolchain produces Linux ARM64 executables in the Debian VM.
  Use it by default. It needs neither a device sysroot nor a global install.
  If it fails, fix the cross-build rather than silently falling back to native
  compilation. Native builds are only a deliberate diagnostic fallback: the
  device has about 352 MiB usable RAM and needs `--target ZeroChord -j1`.
- Keep the playable preview running during development where possible. Use
  isolated test settings, preserve the user's current setup, and restore the
  launcher on exit. Reopen the updated preview promptly after the focused check.
- Package once at the end when packaging is in scope. Capture only screenshots
  needed to verify or document the changed UI; avoid unrelated release work.
- Give concise progress updates about actual results or blockers. Report what
  was checked and any remaining limit without claiming hands-on listening or
  hardware validation that did not happen.
