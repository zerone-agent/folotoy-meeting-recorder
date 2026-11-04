<p align="right"><a href="meeting-agent-prototype.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Meeting recorder agent prototype

The device-specific prototype uses an authenticated Security 2 BLE session to transfer Wi-Fi, service credentials, CA and an initial time. The computer verifies the configured server's TLS certificate and compares its HTTPS Date to its own clock before sending the configuration. The prototype build can start setup and accept its time automatically; no button is needed. Recording storage is never formatted by this flow.

`tools/meeting_provision.py CONFIG --watch` scans for a unique provisioning service, authenticates, submits the configuration and keeps watching for a later reboot. It requires an activated ESP-IDF 5.5.3 environment, BLE dependencies, `MEETING_TEST_PAIRING_CODE`, and `MEETING_TEST_AUTO_CONFIRM=1`. Supply the credentials through a protected environment and keep the configuration file outside the repository with owner-only permissions. Stop the watcher with Ctrl-C. Multiple matching devices are not selected automatically. A successful BLE receipt means durable configuration was acknowledged; Wi-Fi connectivity and upload require separate checks.

The watcher distinguishes discovery failures from incomplete provisioning, reports repeated failures only once, and does not print exception details or credentials. It does not send configuration when the name mismatches; confirm the target instead of overriding its name.

This is a single-device test mechanism, not production onboarding: the fixed pairing value is embedded in that device's test firmware, and the device has neither secure boot nor flash encryption. Someone able to modify or read its firmware or flash is outside this prototype's protection. The final Agent onboarding, device identity, time bootstrap and credential lifecycle still require design and validation. A watcher is also required after reboot until the firmware can re-establish trusted time independently; a running watcher is not proof of completed upload.
