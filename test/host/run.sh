#!/usr/bin/env bash
# Build and run the InkWriter host unit tests with a plain host compiler. The
# editor core is Arduino-free, so no device toolchain or inkkit checkout is
# needed.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"

CXX="${CXX:-g++}"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

echo "Compiling host tests (CXX=$CXX)"
"$CXX" -std=gnu++2a -Wall -Wextra -Werror \
  -I"$root/src" \
  "$root/src/core/Editor.cpp" \
  "$root/src/core/Session.cpp" \
  "$here/test_core.cpp" \
  -o "$out/test_core"

"$out/test_core"
