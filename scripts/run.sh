#!/usr/bin/env bash
#
# Compile a C file with bomi, check the ARM64 it prints is valid assembly,
# then link and run the program and print its answer (what main returns).
#
#   scripts/run.sh                  runs tests/input.txt
#   scripts/run.sh file.c           runs the given file
#   scripts/run.sh -s file.c        also prints the generated assembly
#
# Steps:
#   1. bomi <file>  ->  <name>.s          bomi writes it (and <name>.ll); the AST it prints goes to <name>.log
#   2. clang -c <name>.s -o <name>.o      assemble only: checks the .s is valid ARM64
#   3. clang <name>.o -o <name>           link
#   4. ./<name>, then print its answer (the exit code)
#
# bomi is rebuilt first (incremental), so the run always uses the current sources.
# <name>.s, <name>.ll, <name>.o, <name>.log (AST) and the program are kept in <build dir>/run/.
#
# Exit code: 0 if the program ran (whatever its answer), 1 if bomi, the assembler or the linker failed.
# The answer is main's exit code, so only its low 8 bits survive: 300 shows as 44, -1 as 255.
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

name="$(basename "$src")"
name="${name%.*}"
src_abs="$(cd "$(dirname "$src")" && pwd)/$(basename "$src")"
asm="$OUT/$name.s"
obj="$OUT/$name.o"
bin="$OUT/$name"

# --- 1. bomi: C -> <name>.s (and <name>.ll) ----------------------------------
# bomi writes its files in the directory it runs in, so run it in $OUT;
# what it prints (the AST, or the error) goes to <name>.log
if ! (cd "$OUT" && "$BOMI" "$src_abs") > "$OUT/$name.log" 2>&1; then
    echo "FAIL  $src: bomi reported an error" >&2
    { grep -i 'error' "$OUT/$name.log" || tail -n 3 "$OUT/$name.log"; } | sed 's/^/      /' >&2
    exit 1
fi

if [ "$show_asm" = 1 ]; then
    cat "$asm"
    echo
fi

# --- 2. assemble: is it valid ARM64? ----------------------------------------
# clang -c only runs the assembler, so any error here is in the .s bomi made
if clang -c -x assembler "$asm" -o "$obj" 2> "$OUT/$name.as.log"; then
    echo "OK    $src: valid ARM64 assembly  ($asm)"
else
    echo "FAIL  $src: invalid ARM64 assembly  ($asm)" >&2
    sed 's/^/      /' "$OUT/$name.as.log" >&2
    exit 1
fi

# --- 3. link ----------------------------------------------------------------
if ! clang "$obj" -o "$bin" 2> "$OUT/$name.ld.log"; then
    echo "FAIL  $src: linking failed" >&2
    sed 's/^/      /' "$OUT/$name.ld.log" >&2
    exit 1
fi

# --- 4. run -----------------------------------------------------------------
set +e
"$bin"
code=$?
set -e

echo "answer: $code"
