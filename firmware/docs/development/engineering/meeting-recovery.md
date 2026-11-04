<p align="right"><strong>English</strong> · <a href="meeting-recovery.zh_CN.md">简体中文</a></p>

# Read-only meeting recording recovery

This experimental maintenance path reads a stopped device's partition table, NVS and recording partition; it does not issue flash write or erase commands. NVS may contain the recording identities and snapshots needed for recovery. Entering the serial bootloader interrupts normal operation. The tool refuses unrecognized layouts, encrypted partition flags, invalid partition checksums and existing output directories. It cannot prove the device is factory-new, establish trusted time, or repair a damaged filesystem.

Activate the pinned ESP-IDF environment with its `esptool` dependencies, connect the stopped device and run from the firmware directory:

```sh
python3 tools/meeting_device_export.py --port /dev/DEVICE_PORT /absolute/path/to/new-private-recovery-directory
```

The new directory is created with owner-only access. It contains the raw partition table, `nvs.bin`, `storage.bin` and, when LittleFS mounts successfully, extracted files under `recordings/`. NVS may contain Wi-Fi and device credentials as well as recording metadata; raw images and recordings can contain private audio and cryptographic material. Store them securely. If export fails, retain the raw image and any partial output for investigation; the device may remain in bootloader mode and require a restart. The tool checks the table's ESP-IDF MD5 checksum for accidental corruption, not authenticity. It requests that the device stay in its bootloader between serial reads; one hardware export completed all three reads, but this does not prove atomicity under every failure. Stop recording and ensure no other process uses the device before starting.

The reader is compiled with `LFS_READONLY`. One hardware export extracted 28 files; a 30-minute recording matched a previous read-only backup by hash and a 10-second recording was also extracted. The device had approximately 1.4 MiB free. Recovery with physical damage, less free space or repeated power loss remains unverified. Never format a failed partition as a troubleshooting step.
