#!/usr/bin/env python3
"""Prepare a one-device maintenance build from an authenticated HTTPS signer.

This writes only public material; the admin token is read from a file and is
never written to the output. It does not flash or initialize a signer.
"""
import argparse
import hashlib
import json
import os
import re
import urllib.error
import urllib.parse
import urllib.request

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None

def prepare(origin, mac, token, opener):
    parsed = urllib.parse.urlsplit(origin)
    if (parsed.scheme != 'https' or not parsed.hostname or parsed.username is not None or
            parsed.password is not None or parsed.path or parsed.query or parsed.fragment or
            not re.fullmatch(r'[A-Za-z0-9.-]+', parsed.hostname) or
            not re.fullmatch(r'[0-9a-fA-F]{12}', mac)):
        raise ValueError('invalid origin or device MAC')
    if not token or any(c.isspace() for c in token):
        raise ValueError('invalid admin token')
    request = urllib.request.Request(origin + '/meeting-service/admin/time-signer',
                                     headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/json'})
    with opener.open(request, timeout=10) as response:
        if response.status != 200:
            raise ValueError('time signer unavailable')
        data = response.read(4097)
    if len(data) > 4096:
        raise ValueError('time signer response too large')
    value = json.loads(data)
    if not isinstance(value, dict) or set(value) != {'origin', 'keyId', 'publicKey'} or value['origin'] != origin:
        raise ValueError('time signer origin or response mismatch')
    public = value['publicKey']
    if not isinstance(public, str) or not re.fullmatch(r'04[0-9a-f]{128}', public):
        raise ValueError('invalid P-256 public key encoding')
    if value['keyId'] != hashlib.sha256(bytes.fromhex(public)).hexdigest():
        raise ValueError('time signer key identifier mismatch')
    digest = hashlib.sha256(origin.encode('utf-8')).hexdigest()
    return (f'CONFIG_MEETING_SUPERVISED_DEVICE_MAC="{mac.lower()}"\n'
            'CONFIG_MEETING_AGENT_TIME_PROVISION=y\n'
            f'CONFIG_MEETING_AGENT_TIME_ORIGIN_SHA256="{digest}"\n'
            f'CONFIG_MEETING_AGENT_TIME_PUBLIC_KEY="{public}"\n')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--origin', required=True)
    parser.add_argument('--device-mac', required=True, help='12 hex digits')
    parser.add_argument('--token-file', required=True)
    parser.add_argument('--output', required=True, help='new sdkconfig defaults file')
    args = parser.parse_args()
    with open(args.token_file, encoding='utf-8') as file:
        token = file.read().strip()
    result = prepare(args.origin, args.device_mac, token, urllib.request.build_opener(NoRedirect()))
    fd = os.open(args.output, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    with os.fdopen(fd, 'w', encoding='utf-8') as file:
        file.write(result)
    print('Prepared one-device public-key maintenance configuration:', args.output)

if __name__ == '__main__':
    main()
