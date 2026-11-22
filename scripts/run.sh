#!/usr/bin/env bash
#
# Compile a C file with bomi, assemble the ARM64 it prints and run the program.
#
#   scripts/run.sh                  runs tests/input.txt
#   scripts/run.sh file.c           runs the given file
#   scripts/run.sh -s file.c        also prints the generated assembly
#
# Steps:
#   1. bomi <file>  >  <name>.s       stdout is the assembly, stderr the AST and IR
#   2. clang <name>.s -o <name>       assemble and link
#   3. ./<name>, then print its exit code
#
# bomi is rebuilt first (incremental), so the run always uses the current sources.
# <name>.s, <name>.log (AST + IR) and the program are kept in <build dir>/run/ to look at.
#
# Exit code: the program's own exit code, or 1 if bomi or clang failed.
# Only the low 8 bits of an exit code survive: 300 shows as 44, -1 as 255.
#
# Environment:
#   BOMI_BUILD_DIR  CMake build directory   (default: cmake-build-debug next to the repo)

set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BOMI_BUILD_DIR:-$REPO/../cmake-build-debug}"

die() { echo "error: $*" >&2; exit 1; }

# --- arguments --------------------------------------------------------------
show_asm=0
if [ "${1:-}" = "-s" ]; then
    show_asm=1
    shift
fi

src="${1:-$REPO/tests/input.txt}"
[ -f "$src" ] || die "file not found: $src"

command -v clang > /dev/null || die "clang not found: install the command line tools (xcode-select --install)"

# --- build bomi -------------------------------------------------------------
[ -f "$BUILD_DIR/CMakeCache.txt" ] \
    || die "no CMake build in $BUILD_DIR: build the project once (e.g. in CLion) or set BOMI_BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

# use the same cmake that configured the build dir (CLion ships its own)
CMAKE="$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt")"
[ -x "$CMAKE" ] || CMAKE="$(command -v cmake || true)"
[ -n "$CMAKE" ] || die "cmake not found"

OUT="$BUILD_DIR/run"
mkdir -p "$OUT"

if ! "$CMAKE" --build "$BUILD_DIR" --target bomi > "$OUT/build.log" 2>&1; then
    grep -E 'error' "$OUT/build.log" >&2 || tail -20 "$OUT/build.log" >&2
    die "building bomi failed (full log: $OUT/build.log)"
fi
BOMI="$BUILD_DIR/bomi"
[ -x "$BOMI" ] || die "bomi binary not found at $BOMI"

name="$(basename "${src%.*}")"
asm="$OUT/$name.s"
bin="$OUT/$name"

# --- 1. bomi: C -> assembly -------------------------------------------------
# bomi prints lowering errors to stdout and parse errors to stderr
if ! "$BOMI" "$src" > "$asm" 2> "$OUT/$name.log"; then
    echo "bomi failed on $src:" >&2
    { tail -n 3 "$asm"; grep -i 'error' "$OUT/$name.log" || true; } | sed 's/^/  /' >&2
    exit 1
fi

if [ "$show_asm" = 1 ]; then
    cat "$asm"
    echo
fi

# --- 2. clang: assemble and link --------------------------------------------
if ! clang "$asm" -o "$bin" 2> "$OUT/$name.clang.log"; then
    echo "clang could not assemble $asm:" >&2
    sed 's/^/  /' "$OUT/$name.clang.log" >&2
    exit 1
fi

# --- 3. run -----------------------------------------------------------------
set +e
"$bin"
code=$?
set -e

echo "$name exited with $code"
exit "$code"
