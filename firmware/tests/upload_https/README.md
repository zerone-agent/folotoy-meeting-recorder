<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Local HTTPS upload integration test

This optional test checks that the host transport rejects the wrong CA and the firmware rejects a receipt with a tampered digest. It also runs the firmware's SCR1 scan, projection, hashing, chunk protocol and receipt validation against the repository's server routes over localhost HTTPS and a temporary SQLite database. It simulates a transfer failure halfway through the blocks, kills and restarts the server, drops the final confirmation response, then restarts the server again and verifies an idempotent retry.

A separate database simulates a legacy server without chunk windows. After the status response exceeds the device buffer, the test interrupts the upload and retries in the same client process, verifying that only unconfirmed chunks are sent before final confirmation. The cursor is volatile and is lost on reboot.

Another run removes a previously acknowledged chunk from the temporary database. Finalization must reject the missing chunk, clear the cursor, and require a full retransmission before confirmation.

Prerequisites: Node.js supported by the server, installed `server/node_modules`, a C compiler, curl development files, OpenSSL development files and `openssl` CLI, and the ESP-IDF 5.5.3 checkout. Set `IDF_PATH`; set `OPENSSL_PREFIX` if `brew --prefix openssl@3` does not locate the headers.

```sh
export IDF_PATH=/path/to/esp-idf-5.5.3
export OPENSSL_PREFIX=/path/to/openssl
./firmware/tools/test_upload_https.sh /absolute/path/to/recording.dat
```

The input must be a valid 90,000-frame SCR1 file, such as an authorized read-only export of a 30-minute test recording. It must be large enough to span at least two 4,096-byte chunks. The test does not modify the input. Its test credentials, certificate and SQLite data are created under a private temporary directory and deleted at exit. Output reports counts and results, not audio or credentials.

The host test uses libcurl and OpenSSL for transport and SHA-256; it does not exercise the device's ESP-TLS, Wi-Fi stack, flash, or confirmation-driven cleanup. A clean run is not evidence that device upload or power-loss recovery has passed.
