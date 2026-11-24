#!/bin/sh
# Release-package device check. Usage: install_release_device.sh push|install|install-interactive|test
#   push     copy dist/zerochord_<version>_arm64.deb and the installed-app test to the device
#   install  use the Cardputer workspace sudo helper when configured
#   install-interactive  use a device terminal for the sudo prompt
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
install|install-interactive)
    workspace_root=$(CDPATH= cd -- "$zerochord_root/../.." && pwd)
    if test "$action" = install && test "$ZEROCHORD_SSH_HOST" = cardputer &&
        test "${ZEROCHORD_SSH_CONFIG:-}" -ef "$workspace_root/ssh_config" &&
        test -x "$workspace_root/cardputer-sudo" && test -r "$workspace_root/.env.cardputer"; then
        # Resolve the path as the SSH user before passing it to the privileged helper.
        package_path=$(REMOTE_CMD="cd build/release && sha256sum -c $deb.sha256 >&2 &&
            printf '%s/%s\n' \"\$(pwd -P)\" '$deb'" remote_sh "$@")
        "$workspace_root/cardputer-sudo" dpkg -i "$package_path"
    elif test "$action" = install-interactive || test -t 0; then
        ssh -t "$@" "$ZEROCHORD_SSH_HOST" "cd \"$remote/build/release\" &&
            sha256sum -c $deb.sha256 && sudo dpkg -i $deb"
    else
        REMOTE_CMD="cd build/release && sha256sum -c $deb.sha256 && sudo -n dpkg -i $deb" remote_sh "$@"
    fi
    REMOTE_CMD="systemctl --user restart APPLaunch.service &&
        dpkg-query -W -f='installed \${Package} \${Version}\n' zerochord" remote_sh "$@"
    ;;
test)
    REMOTE_CMD="python3 scripts/test_installed.py $version" remote_sh "$@"
    mkdir -p build/installed-acceptance
    ssh "$@" "$ZEROCHORD_SSH_HOST" "cat \"$remote/build/installed-acceptance/result.json\"" \
        > build/installed-acceptance/result.json
    echo "Fetched build/installed-acceptance/result.json"
    ;;
*) echo "usage: $0 push|install|install-interactive|test" >&2; exit 2 ;;
esac
