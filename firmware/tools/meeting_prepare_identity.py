#!/usr/bin/env python3
"""Create a version 2 device configuration from a server registration.

Input files contain the device token and must be protected. This tool never
prints their contents and refuses to replace an existing output file.
"""
import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import re

from meeting_config_frames import MAX_CONFIG_BYTES
from meeting_provision import review_values

BASE_FIELDS = {'version', 'ssid', 'password', 'origin', 'deviceToken', 'caPem', 'caSha256', 'utc', 'timezoneMinutes'}


def _parse(data):
    def pairs(items):
        out = {}
        for key, value in items:
            if key in out:
                raise ValueError('duplicate JSON key')
            out[key] = value
        return out
    value = json.loads(data, object_pairs_hook=pairs)
    if not isinstance(value, dict):
        raise ValueError('expected JSON object')
    return value


def prepare(base_bytes, registration_bytes):
    if not base_bytes or len(base_bytes) > MAX_CONFIG_BYTES or not registration_bytes or len(registration_bytes) > 4096:
        raise ValueError('input size invalid')
    base, registration = _parse(base_bytes), _parse(registration_bytes)
    if set(base) != BASE_FIELDS or type(base.get('version')) is not int or base['version'] != 1:
        raise ValueError('expected a version 1 base configuration')
    if set(registration) != {'id', 'token', 'credentialId'}:
        raise ValueError('invalid registration fields')
    device, token, credential = registration['id'], registration['token'], registration['credentialId']
    if not isinstance(device, str) or not re.fullmatch(r'[A-Za-z0-9_-]{1,100}', device):
        raise ValueError('invalid device identifier')
    if (not isinstance(token, str) or len(token) < 32 or len(token) > 512 or
            any(ord(ch) <= 32 or ord(ch) >= 127 for ch in token)):
        raise ValueError('invalid device credential')
    if not isinstance(credential, str) or not re.fullmatch(r'[a-f0-9]{64}', credential):
        raise ValueError('invalid credential identifier')
    if not hmac.compare_digest(hashlib.sha256(token.encode('ascii')).hexdigest(), credential):
        raise ValueError('credential identifier does not match token')
    if not isinstance(base['deviceToken'], str) or not hmac.compare_digest(base['deviceToken'], token):
        raise ValueError('base configuration credential does not match registration')
    review_values(base_bytes)
    base['version'] = 2
    base['deviceId'] = device
    base['credentialId'] = credential
    encoded = json.dumps(base, ensure_ascii=True, separators=(',', ':')).encode()
    if len(encoded) > MAX_CONFIG_BYTES:
        raise ValueError('configuration too large')
    return encoded


def write_new(path, data):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        parent = os.open(Path(path).parent, os.O_RDONLY)
        try: os.fsync(parent)
        finally: os.close(parent)
    except BaseException:
        # A partial file must not be used. No preexisting file is overwritten.
        try: os.unlink(path)
        except OSError: pass
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base', type=Path)
    parser.add_argument('registration', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    data = prepare(args.base.read_bytes(), args.registration.read_bytes())
    write_new(args.output, data)
    print('Version 2 configuration prepared')


if __name__ == '__main__': main()
