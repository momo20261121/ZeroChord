#!/bin/sh
# Copy a locally cross-built app; no compilation or test suites on the device.
set -eu
zerochord_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$zerochord_root"
zerochord_binary=build/cross-device/M5CardputerZero-ZeroChord
test -x "$zerochord_binary"
zerochord_hash=$(shasum -a 256 "$zerochord_binary" | cut -d ' ' -f 1)
: "${ZEROCHORD_SSH_HOST:?Set ZEROCHORD_SSH_HOST; see .env.example}"
zerochord_remote_dir=${ZEROCHORD_REMOTE_DIR:-ZeroChord}
# Encode the directory argument so SSH's remote shell cannot interpret it.
zerochord_remote_encoded=$(printf '%s' "$zerochord_remote_dir" | base64 | tr -d '\n')
set -- -o LogLevel=ERROR
if test -n "${ZEROCHORD_SSH_CONFIG:-}"; then
    set -- "$@" -F "$ZEROCHORD_SSH_CONFIG"
fi
scp -q "$@" "$zerochord_binary" \
    "$ZEROCHORD_SSH_HOST:$zerochord_remote_dir/build/device/M5CardputerZero-ZeroChord.next"
ssh "$@" "$ZEROCHORD_SSH_HOST" sh -s -- "$zerochord_hash" "$zerochord_remote_encoded" <<'REMOTE'
set -eu
zerochord_remote_dir=$(printf '%s' "$2" | base64 -d)
cd "$zerochord_remote_dir"
zerochord_root=$(pwd)
binary=build/device/M5CardputerZero-ZeroChord
test "$(sha256sum "$binary.next" | cut -d ' ' -f 1)" = "$1"
chmod 755 "$binary.next"
if systemctl --user is-active --quiet zerochord-preview.service; then
    systemctl --user stop zerochord-preview.service
fi
cp -p "$binary" "$binary.previous"
mv "$binary.next" "$binary"
systemd-run --user --unit=zerochord-preview --collect --service-type=exec \
    /bin/sh "$zerochord_root/scripts/run_device_preview.sh"
sleep 2
if systemctl --user is-active --quiet zerochord-preview.service &&
   pgrep -f '^build/device/M5CardputerZero-ZeroChord' >/dev/null; then
    printf 'Cross-built ZeroChord is running. Previous binary: %s.previous\n' "$binary"
else
    journalctl --user -u zerochord-preview.service -n 12 --no-pager
    if systemctl --user is-active --quiet zerochord-preview.service; then
        systemctl --user stop zerochord-preview.service
    fi
    mv "$binary.previous" "$binary"
    systemd-run --user --unit=zerochord-preview --collect --service-type=exec \
        /bin/sh "$zerochord_root/scripts/run_device_preview.sh"
    printf 'Launch failed; restored the previous app.\n' >&2
    exit 1
fi
REMOTE
