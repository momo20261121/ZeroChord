#!/bin/sh
# Release-package device check. Usage: install_release_device.sh push|install|test
#   push     copy dist/zerochord_<version>_arm64.deb and the installed-app test to the device
#   install  sudo dpkg -i on the device; asks for the device password on a terminal
#            (run it yourself with "! sh scripts/install_release_device.sh install")
#   test     run scripts/test_installed.py against the installed package; fetch result.json
# Reads the device from .env variables (see .env.example); nothing compiles on the device.
set -eu
action=${1:-}
zerochord_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$zerochord_root"
: "${ZEROCHORD_SSH_HOST:?Set ZEROCHORD_SSH_HOST; see .env.example}"
version=$(cat VERSION.txt)
deb=zerochord_${version}_arm64.deb
test -f "dist/$deb" || { echo "dist/$deb not found; run scripts/package.py first" >&2; exit 1; }
(cd dist && shasum -a 256 -c "$deb.sha256" >/dev/null)
remote=${ZEROCHORD_REMOTE_DIR:-ZeroChord}
case "$remote" in /*|~*) ;; *) remote=\$HOME/$remote ;; esac
set -- -o LogLevel=ERROR
if test -n "${ZEROCHORD_SSH_CONFIG:-}"; then set -- "$@" -F "$ZEROCHORD_SSH_CONFIG"; fi
remote_sh() { ssh "$@" "$ZEROCHORD_SSH_HOST" "cd \"$remote\" && $REMOTE_CMD"; }
case "$action" in
push)
    REMOTE_CMD='mkdir -p build/release scripts' remote_sh "$@"
    ssh "$@" "$ZEROCHORD_SSH_HOST" "cat > \"$remote/build/release/$deb\"" < "dist/$deb"
    ssh "$@" "$ZEROCHORD_SSH_HOST" "cat > \"$remote/build/release/$deb.sha256\"" < "dist/$deb.sha256"
    ssh "$@" "$ZEROCHORD_SSH_HOST" "cat > \"$remote/scripts/test_installed.py\"" < scripts/test_installed.py
    REMOTE_CMD="cd build/release && sha256sum -c $deb.sha256" remote_sh "$@"
    ;;
install)
    # -t gives sudo a terminal for its password prompt.
    ssh -t "$@" "$ZEROCHORD_SSH_HOST" "cd \"$remote/build/release\" && sha256sum -c $deb.sha256 &&
        sudo dpkg -i $deb && systemctl --user restart APPLaunch.service &&
        dpkg-query -W -f='installed \${Package} \${Version}\n' zerochord"
    ;;
test)
    REMOTE_CMD="python3 scripts/test_installed.py $version" remote_sh "$@"
    mkdir -p build/installed-acceptance
    ssh "$@" "$ZEROCHORD_SSH_HOST" "cat \"$remote/build/installed-acceptance/result.json\"" \
        > build/installed-acceptance/result.json
    echo "Fetched build/installed-acceptance/result.json"
    ;;
*) echo "usage: $0 push|install|test" >&2; exit 2 ;;
esac
