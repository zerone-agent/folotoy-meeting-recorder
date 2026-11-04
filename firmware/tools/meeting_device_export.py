#!/usr/bin/env python3
"""Read an ESP32-C3 partition into a private directory and export its LittleFS files.

The device enters its serial bootloader. Stop recording before running this tool.
No flash write or erase command is issued. Images can contain private audio and keys.
"""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys

TABLE_OFFSET = 0x8000
TABLE_SIZE = 0x1000
STORAGE_OFFSET = 0x310000
STORAGE_SIZE = 0x4F0000
NVS_OFFSET = 0x9000
NVS_SIZE = 0x6000


def verify_table(raw):
    if len(raw) != TABLE_SIZE:
        raise ValueError("partition table read is incomplete")
    found = {"storage": [], "nvs": []}
    digest = hashlib.md5()  # ESP-IDF partition-table checksum, not a signature
    verified = False
    for pos in range(0, len(raw), 32):
        row = raw[pos:pos + 32]
        if row[:16] == b"\xeb\xeb" + b"\xff" * 14:
            if row[16:] != digest.digest():
                raise ValueError("partition table checksum mismatch")
            verified = True
            break
        magic, kind, subtype, offset, size, label, flags = struct.unpack("<HBBII16sI", row)
        if magic != 0x50AA:
            raise ValueError("partition table has no verified checksum")
        digest.update(row)
        label = label.rstrip(b"\x00")
        if label in (b"storage", b"nvs"):
            found[label.decode("ascii")].append((kind, subtype, offset, size, flags))
    if (not verified or found["storage"] != [(1, 0x83, STORAGE_OFFSET, STORAGE_SIZE, 0)]
            or found["nvs"] != [(1, 0x02, NVS_OFFSET, NVS_SIZE, 0)]):
        raise ValueError("storage or NVS layout or flags do not match this tool")
    return STORAGE_OFFSET, STORAGE_SIZE


def read_device(port, target, runner=subprocess.run, baud=115200):
    if not isinstance(baud, int) or not 115200 <= baud <= 921600:
        raise ValueError("baud must be between 115200 and 921600")
    target.mkdir(mode=0o700)  # fails rather than overwrite any previous recovery
    previous_umask = os.umask(0o077)
    try:
        table = target / "partition-table.bin"
        image = target / "storage.bin"
        base = [sys.executable, "-m", "esptool", "--chip", "esp32c3", "--port", port,
                "--baud", str(baud)]
        # Keep the bootloader active across reads so the application cannot
        # resume recording or cleanup between the NVS and storage captures.
        runner(base + ["--before", "default_reset", "--after", "no_reset", "read_flash",
                       hex(TABLE_OFFSET), hex(TABLE_SIZE), str(table)], check=True)
        offset, size = verify_table(table.read_bytes())
        nvs = target / "nvs.bin"
        runner(base + ["--before", "no_reset", "--after", "no_reset", "read_flash",
                       hex(NVS_OFFSET), hex(NVS_SIZE), str(nvs)], check=True)
        if nvs.stat().st_size != NVS_SIZE:
            raise ValueError("NVS read is incomplete")
        runner(base + ["--before", "no_reset", "--after", "hard_reset", "read_flash",
                       hex(offset), hex(size), str(image)], check=True)
        if image.stat().st_size != size:
            raise ValueError("storage read is incomplete")
        return image
    finally:
        os.umask(previous_umask)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="serial port for the stopped device")
    parser.add_argument("--baud", type=int, default=115200, help="serial speed (115200–921600, default 115200)")
    parser.add_argument("output", type=Path, help="new private output directory")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    lfs = root / "managed_components/joltwallet__littlefs/src/littlefs"
    try:
        image = read_device(args.port, args.output, baud=args.baud)
        executable = args.output / "meeting-lfs-export"
        subprocess.run([os.environ.get("CC", "cc"), "-D_DEFAULT_SOURCE", "-DLFS_READONLY", "-std=c11",
                        "-I", str(lfs), str(root / "tools/meeting_lfs_export.c"), str(lfs / "lfs.c"),
                        str(lfs / "lfs_util.c"), "-o", str(executable)], check=True)
        subprocess.run([str(executable), str(image), str(args.output / "recordings")], check=True)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"Read-only export incomplete: {exc}. Retain any image already read.", file=sys.stderr)
        return 1
    print("Export complete. The original image and extracted files contain sensitive data.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
