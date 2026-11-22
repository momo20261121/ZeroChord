#!/bin/sh
# Run from the user-owned development build; restore APPLaunch after exit.
set -eu
zerochord_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$zerochord_root"
test -x build/device/M5CardputerZero-ZeroChord
trap 'systemctl --user start APPLaunch.service' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
systemctl --user stop APPLaunch.service
# A launcher that was asleep leaves brightness at zero. With the launcher
# stopped, keyboard input cannot wake it; wake this development session here.
# Keep an existing nonzero brightness and leave normal installed-app policy to
# APPLaunch. The device's video group has permission to write this control.
zerochord_backlight=/sys/class/backlight/backlight/brightness
if test -r "$zerochord_backlight" && test "$(cat "$zerochord_backlight")" -eq 0; then
    printf '80\n' > "$zerochord_backlight"
fi
build/device/M5CardputerZero-ZeroChord "$@"
