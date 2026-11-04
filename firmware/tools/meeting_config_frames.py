"""Encode authenticated meeting configuration frames for the BLE client.

The transport must encrypt each frame in one Security 2 session and reset the
device receiver after disconnect. The caller owns and protects config bytes.
"""

import struct
from collections.abc import Iterator

MAX_CONFIG_BYTES = 6144
MAX_FRAGMENT_BYTES = 400  # frame + Security 2 GCM tag fits the 480-byte BLE limit


def frames(payload: bytes, fragment_size: int = MAX_FRAGMENT_BYTES) -> Iterator[bytes]:
    if not isinstance(payload, bytes) or not 0 < len(payload) <= MAX_CONFIG_BYTES:
        raise ValueError("configuration length is invalid")
    if not isinstance(fragment_size, int) or not 0 < fragment_size <= MAX_FRAGMENT_BYTES:
        raise ValueError("fragment size is invalid")
    yield b"\x01" + struct.pack("<H", len(payload))
    for offset in range(0, len(payload), fragment_size):
        yield b"\x02" + struct.pack("<H", offset) + payload[offset:offset + fragment_size]
    yield b"\x03"
