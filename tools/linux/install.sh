#!/usr/bin/env bash
# Copy the built client files next to an existing wow.exe.
# Usage: tools/linux/install.sh <client dir>
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out_dir="$repo_root/bin/Win"
files=(WowClient64.exe WowClient64.pdb XvidDecoder.dll)

case "${1:-}" in
    -h | --help)
        sed -n '2,3p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
        exit 0
        ;;
esac
if [ $# -ne 1 ]; then
    echo "usage: tools/linux/install.sh <client dir>" >&2
    exit 2
fi
dest="$1"

if [ ! -d "$dest" ]; then
    echo "error: not a directory: $dest" >&2
    exit 1
fi
if [ -z "$(find "$dest" -maxdepth 1 -iname wow.exe -print -quit)" ]; then
    echo "error: no wow.exe in $dest; refusing to install" >&2
    exit 1
fi

missing=0
for f in "${files[@]}"; do
    if [ ! -f "$out_dir/$f" ]; then
        echo "error: not built: $out_dir/$f (run tools/linux/build.sh)" >&2
        missing=1
    fi
done
[ "$missing" -eq 0 ] || exit 1

# Only these three names are ever written; wow.exe and everything else is left alone.
for f in "${files[@]}"; do
    cp -- "$out_dir/$f" "$dest/$f"
    echo "installed $f"
done
