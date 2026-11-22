#!/bin/sh
# Install a local pre-push hook so the dev branch can never be pushed.
# Hooks are not versioned; /publish runs this each time (it is idempotent).
set -eu
zerochord_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
hook=$(git -C "$zerochord_root" rev-parse --git-path hooks/pre-push)
case "$hook" in /*) ;; *) hook=$zerochord_root/$hook ;; esac
if test -e "$hook" && ! grep -q 'ZeroChord dev guard' "$hook"; then
    echo "Existing pre-push hook is not ours; not overwriting: $hook" >&2
    exit 1
fi
cat > "$hook" <<'HOOK'
#!/bin/sh
# ZeroChord dev guard: the dev branch stays local and is never pushed.
while read -r local_ref local_sha remote_ref remote_sha; do
    if test "$local_ref" = refs/heads/dev || test "$remote_ref" = refs/heads/dev; then
        echo "Refusing to push: the dev branch is local-only." >&2
        exit 1
    fi
done
exit 0
HOOK
chmod 755 "$hook"
echo "dev push guard installed"
