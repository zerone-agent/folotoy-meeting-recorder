import hashlib
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).parents[2] / 'tools'))
import meeting_prepare_identity as prepare


class PrepareIdentityTest(unittest.TestCase):
    def setUp(self):
        self.token = 'x' * 40
        self.base = {'version': 1, 'ssid': 'wifi', 'password': 'pass', 'origin': 'https://example.com',
                     'deviceToken': self.token, 'caPem': 'CA', 'caSha256': 'a' * 64,
                     'utc': 1800000000, 'timezoneMinutes': 480}
        self.registration = {'id': 'device_1', 'token': self.token,
                             'credentialId': hashlib.sha256(self.token.encode()).hexdigest()}
    def encode(self, value): return json.dumps(value).encode()
    @mock.patch.object(prepare, 'review_values')
    def test_valid_registration_and_new_file_only(self, review):
        data = prepare.prepare(self.encode(self.base), self.encode(self.registration))
        result = json.loads(data)
        self.assertEqual((result['version'], result['deviceId'], result['credentialId']),
                         (2, 'device_1', self.registration['credentialId']))
        review.assert_called_once()
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'config.json'
            prepare.write_new(path, data)
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): prepare.write_new(path, data)
            self.assertEqual(path.read_bytes(), data)
    @mock.patch.object(prepare, 'review_values')
    def test_rejects_wrong_token_hash_and_ambiguous_input(self, _review):
        for field, value in [('id', '../wrong'), ('token', 'z' * 40), ('credentialId', '0' * 64)]:
            bad = dict(self.registration, **{field: value})
            with self.assertRaises(ValueError): prepare.prepare(self.encode(self.base), self.encode(bad))
        duplicate = self.encode(self.registration)[:-1] + b',"id":"other"}'
        with self.assertRaises(ValueError): prepare.prepare(self.encode(self.base), duplicate)
        base = dict(self.base, version=2)
        with self.assertRaises(ValueError): prepare.prepare(self.encode(base), self.encode(self.registration))


if __name__ == '__main__': unittest.main()
