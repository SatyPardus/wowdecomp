#!/usr/bin/env bash
# Clean-build the client and check the artifacts.
# Usage: tools/linux/check.sh [--no-build]
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out_dir="$repo_root/bin/Win"
exe="$out_dir/WowClient64.exe"
pdb="$out_dir/WowClient64.pdb"
dll="$out_dir/XvidDecoder.dll"

build=1
while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) build=0 ;;
        -h | --help)
            sed -n '2,3p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "error: unknown option: $1" >&2
            exit 2
            ;;
    esac
    shift
done

if [ "$build" -eq 1 ]; then
    "$repo_root/tools/linux/build.sh" --clean
fi

failed=0
check() {
    local name="$1"
    shift
    if "$@" > /dev/null 2>&1; then
        echo "PASS $name"
    else
        echo "FAIL $name"
        failed=1
    fi
}

is_gui_exe() { file -b "$exe" | grep -Eq 'PE32\+ executable.*\(GUI\), x86-64'; }
is_dll() { file -b "$dll" | grep -Eq 'PE32\+ executable.*\(DLL\), x86-64'; }
no_runtime_imports() {
    local imports
    imports="$(llvm-readobj --coff-imports "$exe")" || return 1
    ! grep -Eiq 'Name: (vcruntime140|msvcp140|ucrtbase|api-ms-win-crt-)' <<< "$imports"
}

check "WowClient64.exe is PE32+ GUI x86-64" is_gui_exe
check "WowClient64.exe has no runtime DLL imports" no_runtime_imports
check "WowClient64.pdb exists" test -f "$pdb"
check "XvidDecoder.dll is a PE32+ x86-64 DLL" is_dll
# BugReport.cpp only compiles this string in when DISABLE_SIGNATURE_CHECKS is defined.
signature_checks_off() { grep -aq 'signature check OFF' "$exe"; }
check "WowClient64.exe is built with signature checks off" signature_checks_off
# A section with no memory flags is mapped unreadable, and the client's exe checksum faults on it (see the toolchain file).
all_sections_readable() {
    local sections
    sections="$(llvm-readobj --sections "$exe")" || return 1
    ! grep -q 'Characteristics \[ (0x0)' <<< "$sections"
}
check "WowClient64.exe has no section without memory flags" all_sections_readable

exit "$failed"
