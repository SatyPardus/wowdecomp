#!/usr/bin/env bash
# Configure and build the Windows x64 client on Linux with clang-cl, lld-link, and xwin.
# Usage: tools/linux/build.sh [--clean] [--configure-only] [--signature-checks] [target...]
set -euo pipefail

CRT_VERSION="14.44.17.14"
SDK_VERSION="10.0.26100"
XWIN_DIR="${XWIN_DIR:-$HOME/.local/share/xwin}"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="$repo_root/build/linux-x64"

clean=0
configure_only=0
disable_signature_checks=ON
targets=()

while [ $# -gt 0 ]; do
    case "$1" in
        --clean) clean=1 ;;
        --configure-only) configure_only=1 ;;
        --signature-checks) disable_signature_checks=OFF ;;
        -h | --help)
            sed -n '2,3p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*)
            echo "error: unknown option: $1" >&2
            exit 2
            ;;
        *) targets+=("$1") ;;
    esac
    shift
done

missing=0
for tool in clang-cl lld-link llvm-rc cmake ninja; do
    if ! command -v "$tool" > /dev/null 2>&1; then
        echo "error: '$tool' is not on PATH" >&2
        missing=1
    fi
done
for dir in "$XWIN_DIR/VC/Tools/MSVC/$CRT_VERSION" "$XWIN_DIR/Windows Kits/10/Include/$SDK_VERSION"; do
    if [ ! -d "$dir" ]; then
        echo "error: missing folder: $dir (XWIN_DIR=$XWIN_DIR)" >&2
        missing=1
    fi
done
if [ "$missing" -ne 0 ]; then
    exit 1
fi

if [ "$clean" -eq 1 ]; then
    rm -rf "$build_dir"
fi

cd "$repo_root"
cmake -S . -B "$build_dir" -G Ninja --toolchain cmake/clang-cl-xwin.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DXWIN_DIR="$XWIN_DIR" \
    -DWOWDECOMP_DISABLE_SIGNATURE_CHECKS="$disable_signature_checks"

if [ "$configure_only" -eq 1 ]; then
    exit 0
fi

if [ "${#targets[@]}" -eq 0 ]; then
    targets=(WowClient XvidDecoder)
fi
cmake --build "$build_dir" --target "${targets[@]}"
