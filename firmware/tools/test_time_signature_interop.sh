#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${IDF_PATH:?activate ESP-IDF 5.5.3 first}"
dir="$(mktemp -d /tmp/meeting-time-interop.XXXXXX)"
server_pid=''
container_id=''
cleanup() { if [[ -n "$container_id" ]]; then docker stop --time 1 "$container_id" >/dev/null 2>&1 || true; fi; if [[ -n "$server_pid" ]]; then kill "$server_pid" 2>/dev/null || true; wait "$server_pid" 2>/dev/null || true; fi; rm -rf -- "$dir"; }
trap cleanup EXIT
node "$root/tests/meeting/gen_time_vector.mjs" "$dir"
cmake -S "$IDF_PATH/components/mbedtls/mbedtls" -B "$dir/build" -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF -DUSE_SHARED_MBEDTLS_LIBRARY=OFF > "$dir/build.log" 2>&1
cmake --build "$dir/build" --target mbedcrypto -j 6 >> "$dir/build.log" 2>&1
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"$root/main" -I"$IDF_PATH/components/mbedtls/mbedtls/include" -I"$IDF_PATH/components/json/cJSON" \
 "$root/tests/meeting/test_time_signature.c" "$root/main/meeting_time_client.c" "$root/main/meeting_time_signature.c" "$root/main/meeting_time_anchor.c" "$root/main/meeting_time_response.c" "$root/main/meeting_time_proof.c" "$root/main/meeting_clock.c" "$IDF_PATH/components/json/cJSON/cJSON.c" "$dir/build/library/libmbedcrypto.a" -lm -o "$dir/interop"
"$dir/interop" "$dir"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"$root/main" -I"$IDF_PATH/components/mbedtls/mbedtls/include" -I"$IDF_PATH/components/json/cJSON" \
 "$root/tests/meeting/test_live_time.c" "$root/main/meeting_time_client.c" "$root/main/meeting_time_signature.c" "$root/main/meeting_time_response.c" "$root/main/meeting_time_proof.c" "$root/main/meeting_clock.c" "$IDF_PATH/components/json/cJSON/cJSON.c" "$dir/build/library/libmbedcrypto.a" -lm -o "$dir/live"
if [[ "${MEETING_TEST_PROXY:-0}" == 1 ]]; then export MEETING_TEST_BIND=0.0.0.0; fi
node "$root/tests/meeting/live_time_server.mjs" "$dir" > "$dir/server.log" 2>&1 &
server_pid=$!
for ((i=0;i<100;i++)); do [[ -s "$dir/live-port.txt" ]] && break; kill -0 "$server_pid" 2>/dev/null || break; sleep 0.1; done
if [[ ! -s "$dir/live-port.txt" ]]; then cat "$dir/server.log" >&2; echo 'time server failed to start' >&2; exit 1; fi
"$dir/live" "$dir/live-point.bin" "$(cat "$dir/live-port.txt")"
echo 'Node P-256 to device Mbed TLS signature interop: PASS'
echo 'Real HTTP time route to device time client: PASS'
if [[ "${MEETING_TEST_PROXY:-0}" == 1 ]]; then
    port="$(cat "$dir/live-port.txt")"
    sed "s/127.0.0.1:3443/host.docker.internal:${port}/" "$root/../deploy/nginx-time-proof.conf" > "$dir/default.conf"
    container_id="$(docker run --detach --rm --publish 127.0.0.1::80 --volume "$dir/default.conf:/etc/nginx/conf.d/default.conf:ro" nginx:1.27-alpine)"
    proxy_port="$(docker port "$container_id" 80/tcp | awk -F: 'NR==1{print $NF}')"
    for ((i=0;i<100;i++)); do
        if curl --max-time 1 --silent --output /dev/null "http://127.0.0.1:${proxy_port}/"; then break; fi
        sleep 0.1
    done
    "$dir/live" "$dir/live-point.bin" "$proxy_port"
    echo 'Real Nginx proxy to service and device time client: PASS'
fi
