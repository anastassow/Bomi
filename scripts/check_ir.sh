#!/usr/bin/env bash
#
# Compile source files with bomi and check that the output is valid LLVM IR.
#
#   scripts/check_ir.sh                  checks tests/input.c
#   scripts/check_ir.sh a.c b.c ...      checks the given files
#
# For each file:
#   1. bomi <file>  ->  <name>.ll             (bomi writes it; the AST it prints goes to <name>.log)
#   2. opt -passes=verify -disable-output <file>.ll
#      LLVM's verifier: checks the IR is valid, does not run or compile it.
#
# bomi is rebuilt first (incremental), so the check always uses the current sources.
# The .ll files (and the .s bomi writes next to them) are kept in <build dir>/check_ir/.
#
# Exit code: 0 if every file produced valid LLVM IR, 1 otherwise.
#
# Environment:
#   BOMI_BUILD_DIR  CMake build directory   (default: cmake-build-debug next to the repo)
#   LLVM_BIN        directory containing opt (default: /opt/homebrew/opt/llvm/bin, then PATH)

set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BOMI_BUILD_DIR:-$REPO/../cmake-build-debug}"

die() { echo "error: $*" >&2; exit 1; }

# --- LLVM's verifier --------------------------------------------------------
OPT="${LLVM_BIN:-/opt/homebrew/opt/llvm/bin}/opt"
if [ ! -x "$OPT" ]; then
    OPT="$(command -v opt || true)"
fi
[ -n "$OPT" ] || die "LLVM's opt not found: install LLVM (brew install llvm) or set LLVM_BIN"

# --- build bomi -------------------------------------------------------------
[ -f "$BUILD_DIR/CMakeCache.txt" ] \
    || die "no CMake build in $BUILD_DIR: build the project once (e.g. in CLion) or set BOMI_BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

# use the same cmake that configured the build dir (CLion ships its own)
CMAKE="$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt")"
[ -x "$CMAKE" ] || CMAKE="$(command -v cmake || true)"
[ -n "$CMAKE" ] || die "cmake not found"

OUT="$BUILD_DIR/check_ir"
mkdir -p "$OUT"

if ! "$CMAKE" --build "$BUILD_DIR" --target bomi > "$OUT/build.log" 2>&1; then
    grep -E 'error' "$OUT/build.log" >&2 || tail -20 "$OUT/build.log" >&2
    die "building bomi failed (full log: $OUT/build.log)"
fi
BOMI="$BUILD_DIR/bomi"
[ -x "$BOMI" ] || die "bomi binary not found at $BOMI"

# --- check each file --------------------------------------------------------
check() {
    local src="$1" base ll src_abs
    base="$(basename "$src")"
    base="${base%.*}"
    ll="$OUT/$base.ll"

    [ -f "$src" ] || { echo "FAIL  $src: file not found"; return 1; }
    src_abs="$(cd "$(dirname "$src")" && pwd)/$(basename "$src")"

    # bomi writes <name>.ll and <name>.s in the directory it runs in, so run it in $OUT;
    # what it prints (the AST, or the error) goes to <name>.log
    if ! (cd "$OUT" && "$BOMI" "$src_abs") > "$OUT/$base.log" 2>&1; then
        echo "FAIL  $src: bomi reported an error"
        { grep -i 'error' "$OUT/$base.log" || tail -n 3 "$OUT/$base.log"; } | sed 's/^/      /'
        return 1
    fi

    if "$OPT" -passes=verify -disable-output "$ll" 2> "$OUT/$base.verify"; then
        echo "OK    $src: valid LLVM IR  ($ll)"
    else
        echo "FAIL  $src: invalid LLVM IR  ($ll)"
        sed 's/^/      /' "$OUT/$base.verify"
        return 1
    fi
}

[ $# -gt 0 ] || set -- "$REPO/tests/input.c"

status=0
for src in "$@"; do
    check "$src" || status=1
done
exit $status
