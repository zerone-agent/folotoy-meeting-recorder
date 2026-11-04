import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).parents[2] / 'tools'))
import meeting_revocation_outbox as outbox


class Response:
    def __init__(self, payload, status=200):
        self.payload, self.status = payload, status
    def __enter__(self): return self
    def __exit__(self, *args): return False
    def read(self, limit): return self.payload[:limit]


class RevocationOutboxTest(unittest.TestCase):
    def test_failure_then_idempotent_confirmation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'pending.json'
            outbox.create(path, 'https://example.com', 'device_1', 'a' * 64, 'event_1')
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError):
                outbox.create(path, 'https://example.com', 'device_1', 'a' * 64, 'event_1')
            opener = mock.Mock()
            opener.open.side_effect = TimeoutError('connection interrupted')
            with self.assertRaises(TimeoutError): outbox.retry(path, 'A' * 32, opener=opener)
            self.assertEqual(json.loads(path.read_text())['state'], 'pending')
            opener.open.side_effect = None
            opener.open.return_value = Response(b'{"eventId":"other","revoked":true}')
            with self.assertRaises(ValueError): outbox.retry(path, 'A' * 32, opener=opener)
            self.assertEqual(json.loads(path.read_text())['state'], 'pending')
            opener.open.return_value = Response(b'{"eventId":"event_1","revoked":true}')
            self.assertTrue(outbox.retry(path, 'A' * 32, opener=opener))
            request = opener.open.call_args.args[0]
            self.assertEqual(json.loads(request.data), {'credentialId': 'a' * 64, 'eventId': 'event_1'})
            self.assertEqual(json.loads(path.read_text())['state'], 'confirmed')
            self.assertNotIn('A' * 32, path.read_text())
            opener.open.reset_mock()
            self.assertTrue(outbox.retry(path, 'A' * 32, opener=opener))
            opener.open.assert_not_called()

    def test_rejects_malformed_or_ambiguous_records(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'pending.json'
            for device in ('../other', 'bad/name', ''):
                with self.assertRaises(ValueError): outbox.create(path, 'https://example.com', device, 'a' * 64, 'event')
            outbox.create(path, 'https://example.com', 'device', 'a' * 64, 'event')
            opener = mock.Mock()
            opener.open.return_value = Response(b'{"eventId":"event","eventId":"event","revoked":true}')
            with self.assertRaises(ValueError): outbox.retry(path, 'A' * 32, opener=opener)
            self.assertEqual(json.loads(path.read_text())['state'], 'pending')
            path.write_text('{"state":"confirmed","state":"pending"}')
            with self.assertRaises(ValueError): outbox.retry(path, 'A' * 32, opener=opener)


if __name__ == '__main__': unittest.main()
