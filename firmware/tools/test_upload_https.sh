#!/usr/bin/env bash
# Optional local integration test. The recording never leaves localhost.
set -euo pipefail
if [[ $# != 1 ]]; then
    echo "Usage: $0 /absolute/path/to/30-minute-scr1.dat" >&2
    exit 2
fi
: "${IDF_PATH:?Set IDF_PATH to the ESP-IDF 5.5.3 checkout}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
source_file="$(python3 -c 'from pathlib import Path;import sys;print(Path(sys.argv[1]).resolve(strict=True))' "$1")"
if [[ -z "${OPENSSL_PREFIX:-}" ]]; then
    OPENSSL_PREFIX="$(brew --prefix openssl@3 2>/dev/null || true)"
fi
if [[ -z "${OPENSSL_PREFIX:-}" || ! -f "$OPENSSL_PREFIX/include/openssl/evp.h" ]]; then
    echo "Set OPENSSL_PREFIX to an OpenSSL installation with development headers" >&2
    exit 2
fi
umask 077
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/meeting-https.XXXXXXXX")"
trap 'rm -rf -- "$test_dir"' EXIT
firmware="$repo_root/firmware"
"${CC:-cc}" -D_POSIX_C_SOURCE=200809L -std=c11 -O2 -Wall -Wextra -Werror \
    -I"$firmware/tests/upload_https/host_include" -I"$OPENSSL_PREFIX/include" \
    -I"$firmware/tests/meeting/stubs" -I"$firmware/main" -I"$IDF_PATH/components/json/cJSON" \
    "$firmware/tests/upload_https/client.c" "$firmware/tests/upload_https/sha256_openssl.c" \
    "$firmware/main/meeting_scr1.c" "$firmware/main/meeting_scr1_file.c" \
    "$firmware/main/meeting_snapshot_file.c" "$firmware/main/meeting_upload.c" \
    "$firmware/main/meeting_upload_protocol.c" "$firmware/main/meeting_receipt.c" \
    "$firmware/main/meeting_receipt_json.c" "$IDF_PATH/components/json/cJSON/cJSON.c" \
    -L"$OPENSSL_PREFIX/lib" -lcrypto -lcurl -o "$test_dir/client"
node "$firmware/tests/upload_https/run.mjs" "$source_file" "$test_dir/client" "$repo_root"
