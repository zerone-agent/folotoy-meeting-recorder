#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
dir="$(mktemp -d /tmp/meeting-lfs-test.XXXXXX)"
trap 'rm -rf "$dir"' EXIT
lib="$root/managed_components/joltwallet__littlefs/src/littlefs"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"$lib" "$root/tests/meeting/lfs_fixture.c" "$lib/lfs.c" "$lib/lfs_util.c" -o "$dir/fixture"
"${CC:-cc}" -D_DEFAULT_SOURCE -DLFS_READONLY -std=c11 -Wall -Wextra -Werror -Wno-unused-function -I"$lib" "$root/tools/meeting_lfs_export.c" "$lib/lfs.c" "$lib/lfs_util.c" -o "$dir/exporter"
"$dir/fixture" "$dir/image"
cp "$dir/image" "$dir/original"
"$dir/exporter" "$dir/image" "$dir/result"
cmp "$dir/image" "$dir/original"
printf abc > "$dir/expected"
cmp "$dir/expected" "$dir/result/123.dat"
if "$dir/exporter" "$dir/image" "$dir/result"; then exit 1; fi
ln -s "$dir/image" "$dir/image-link"
if "$dir/exporter" "$dir/image-link" "$dir/link-result"; then exit 1; fi
ln -s "$dir/result" "$dir/output-link"
if "$dir/exporter" "$dir/image" "$dir/output-link"; then exit 1; fi
cmp "$dir/expected" "$dir/result/123.dat"
cmp "$dir/image" "$dir/original"
printf broken > "$dir/bad"
if "$dir/exporter" "$dir/bad" "$dir/bad-result"; then exit 1; fi
python3 -c 'import sys; open(sys.argv[1], "wb").write(b"\xff" * 0x4f0000)' "$dir/erased"
if "$dir/exporter" "$dir/erased" "$dir/erased-result"; then exit 1; fi
