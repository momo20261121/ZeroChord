#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

if ! test -x .venv/bin/cmake || ! test -x .venv/bin/ctest; then
    printf '%s\n' 'Set up the project as described in docs/DEVELOPMENT.md first.' >&2
    exit 1
fi

.venv/bin/cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Release
.venv/bin/cmake --build build/desktop --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-4}"
exec .venv/bin/ctest --test-dir build/desktop --output-on-failure
