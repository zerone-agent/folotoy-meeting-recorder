import asyncio
import hashlib
import contextlib
import io
import json
from email.message import Message
import pathlib
import sys
import unittest
from types import SimpleNamespace
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).parents[2] / "tools"))
import meeting_provision


class Security:
    def __init__(self):
        self.phase = 0

    def security_session(self, response):
        self.phase += 1
        if self.phase == 1:
            self.assert_response(response, None)
            return "hello"
        if self.phase == 2:
            self.assert_response(response, "reply")
            return "proof"
        self.assert_response(response, "reply")
        return None

    @staticmethod
    def assert_response(actual, expected):
        if actual != expected:
            raise AssertionError("wrong handshake response")

    @staticmethod
    def encrypt_data(data):
        return b"E" + data

    @staticmethod
    def decrypt_data(data):
        return data[1:]


class Transport:
    def __init__(self, reject_at=None):
        self.requests = []
        self.reject_at = reject_at

    async def send_data(self, endpoint, message):
        self.requests.append((endpoint, message))
        if endpoint == "prov-session":
            return "reply"
        return "E\x01" if len(self.requests) == self.reject_at else "E\x00"


class ClientTest(unittest.TestCase):
    def test_unique_service_selection_ignores_unrelated_same_name(self):
        target=SimpleNamespace(name=None)
        other=SimpleNamespace(name="PASSPORT-SETUP")
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID.upper()])
        unrelated=SimpleNamespace(service_uuids=["00000000-0000-0000-0000-000000000000"])
        selected=meeting_provision.unique_setup_discovery({"a":(target,service),"b":(other,unrelated)},"PASSPORT-SETUP")
        self.assertEqual(list(selected),["a"])
        self.assertEqual(target.name,"PASSPORT-SETUP")
        with self.assertRaises(RuntimeError):
            meeting_provision.unique_setup_discovery({"a":(target,service),"b":(other,service)},"PASSPORT-SETUP")
        with self.assertRaises(RuntimeError):
            meeting_provision.unique_setup_discovery({},"PASSPORT-SETUP")
        with self.assertRaises(RuntimeError):
            meeting_provision.unique_setup_discovery({"a":(SimpleNamespace(name="other"),service)},"PASSPORT-SETUP")

    def test_current_advertisement_overrides_stale_cached_name(self):
        device=SimpleNamespace(name="soundcore Work")
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID],local_name="PASSPORT-SETUP")
        selected=meeting_provision.unique_setup_discovery({"a":(device,service)},"PASSPORT-SETUP")
        self.assertEqual(list(selected),["a"])
        self.assertEqual(device.name,"PASSPORT-SETUP")
        service.local_name="other"
        with self.assertRaises(RuntimeError):
            meeting_provision.unique_setup_discovery({"a":(device,service)},"PASSPORT-SETUP")

    def test_watch_waits_for_unique_device_and_reuses_scan(self):
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID])
        target=SimpleNamespace(name=None)
        scans=[{}, {"a":(target,service)}]
        observed=[]
        async def scan(**kwargs):
            self.assertEqual(kwargs,{"timeout":5,"return_adv":True})
            return scans.pop(0)
        async def runner(path,name,discovery):
            observed.append((path,name,discovery))
        async def sleep(seconds):
            self.assertEqual(seconds,10)
            if observed:raise asyncio.CancelledError()
        async def execute():
            with self.assertRaises(asyncio.CancelledError):
                await meeting_provision.watch(pathlib.Path("config.json"),"PASSPORT-SETUP",scan,runner,sleep)
        with mock.patch.dict(meeting_provision.os.environ,{"MEETING_TEST_AUTO_CONFIRM":"1","MEETING_TEST_PAIRING_CODE":"123456789012"}):
            asyncio.run(execute())
        self.assertEqual(len(observed),1)
        self.assertEqual(list(observed[0][2]),["a"])

    def test_watch_does_not_send_configuration_to_mismatched_name(self):
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID])
        scans=[{"wrong":(SimpleNamespace(name="soundcore Work"),service)},
               {"right":(SimpleNamespace(name="PASSPORT-SETUP"),service)}]
        observed=[]
        async def scan(**kwargs): return scans.pop(0)
        async def runner(path,name,discovery): observed.append(list(discovery))
        async def sleep(seconds):
            if observed: raise asyncio.CancelledError()
        async def execute():
            with self.assertRaises(asyncio.CancelledError):
                await meeting_provision.watch(pathlib.Path("config.json"),"PASSPORT-SETUP",scan,runner,sleep)
        with mock.patch.dict(meeting_provision.os.environ,{"MEETING_TEST_AUTO_CONFIRM":"1","MEETING_TEST_PAIRING_CODE":"123456789012"}):
            asyncio.run(execute())
        self.assertEqual(observed,[["right"]])

    def test_watch_uses_live_name_when_cached_name_is_stale(self):
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID],local_name="PASSPORT-SETUP")
        async def scan(**kwargs): return {"a":(SimpleNamespace(name="soundcore Work"),service)}
        observed=[]
        async def runner(path,name,discovery): observed.append(discovery["a"][0].name)
        async def sleep(seconds): raise asyncio.CancelledError()
        async def execute():
            with self.assertRaises(asyncio.CancelledError):
                await meeting_provision.watch(pathlib.Path("config.json"),"PASSPORT-SETUP",scan,runner,sleep)
        with mock.patch.dict(meeting_provision.os.environ,{"MEETING_TEST_AUTO_CONFIRM":"1","MEETING_TEST_PAIRING_CODE":"123456789012"}):
            asyncio.run(execute())
        self.assertEqual(observed,["PASSPORT-SETUP"])

    def test_watch_reports_failure_once_without_exception_contents(self):
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID])
        async def scan(**kwargs): return {"a":(SimpleNamespace(name="PASSPORT-SETUP"),service)}
        async def runner(path,name,discovery): raise RuntimeError("private token must not be logged")
        attempts=[]
        async def sleep(seconds):
            attempts.append(seconds)
            if len(attempts)==2: raise asyncio.CancelledError()
        output=io.StringIO()
        async def execute():
            with self.assertRaises(asyncio.CancelledError):
                await meeting_provision.watch(pathlib.Path("config.json"),"PASSPORT-SETUP",scan,runner,sleep)
        with mock.patch.dict(meeting_provision.os.environ,{"MEETING_TEST_AUTO_CONFIRM":"1","MEETING_TEST_PAIRING_CODE":"123456789012"}), contextlib.redirect_stdout(output):
            asyncio.run(execute())
        self.assertEqual(output.getvalue().count("Provisioning did not complete"),1)
        self.assertNotIn("private token",output.getvalue())

    def test_watch_reports_discovery_failure_once_without_device_name(self):
        service=SimpleNamespace(service_uuids=[meeting_provision.SERVICE_UUID])
        async def scan(**kwargs): return {"a":(SimpleNamespace(name="private device name"),service)}
        async def runner(path,name,discovery): self.fail("must not send configuration")
        attempts=[]
        async def sleep(seconds):
            attempts.append(seconds)
            if len(attempts)==2: raise asyncio.CancelledError()
        output=io.StringIO()
        async def execute():
            with self.assertRaises(asyncio.CancelledError):
                await meeting_provision.watch(pathlib.Path("config.json"),"PASSPORT-SETUP",scan,runner,sleep)
        with mock.patch.dict(meeting_provision.os.environ,{"MEETING_TEST_AUTO_CONFIRM":"1","MEETING_TEST_PAIRING_CODE":"123456789012"}), contextlib.redirect_stdout(output):
            asyncio.run(execute())
        self.assertEqual(output.getvalue().count("No unique matching provisioning device"),1)
        self.assertNotIn("private device name",output.getvalue())

    def test_handshake_and_ordered_frames(self):
        transport = Transport()
        asyncio.run(meeting_provision.send_frames(transport, Security(), b"abcdef"))
        self.assertEqual([item[0] for item in transport.requests], ["prov-session", "prov-session", "meeting-config", "meeting-config", "meeting-config"])
        self.assertEqual(transport.requests[2][1].encode("latin-1"), b"E\x01\x06\x00")
        self.assertEqual(transport.requests[3][1].encode("latin-1"), b"E\x02\x00\x00abcdef")
        self.assertEqual(transport.requests[4][1].encode("latin-1"), b"E\x03")

    def test_rejection_stops_before_seal(self):
        transport = Transport(reject_at=4)
        with self.assertRaises(RuntimeError):
            asyncio.run(meeting_provision.send_frames(transport, Security(), b"abcdef"))
        self.assertEqual(len(transport.requests), 4)

    def test_status_commands_are_encrypted(self):
        transport = Transport()
        self.assertEqual(asyncio.run(meeting_provision.command(transport, Security(), b"\x04")), b"\x00")
        self.assertEqual(transport.requests, [("meeting-config", "E\x04")])

    def test_review_values_match_actual_certificate(self):
        fingerprint = hashlib.sha256(b"certificate").hexdigest()
        payload = json.dumps({"caPem": "test", "caSha256": fingerprint, "utc": 1800000000}).encode()
        with mock.patch.object(meeting_provision.ssl, "PEM_cert_to_DER_cert", return_value=b"certificate"):
            self.assertEqual(meeting_provision.review_values(payload), (fingerprint.upper(), 1800000000))
            with self.assertRaises(ValueError):
                meeting_provision.review_values(payload.replace(fingerprint.encode(), b"0" * 64))
            with self.assertRaises(ValueError):
                meeting_provision.review_values(b'{"utc":1,"utc":2}')

    def test_configuration_time_must_be_recent(self):
        meeting_provision.check_freshness(1800000000, 1800000030)
        with self.assertRaises(ValueError):
            meeting_provision.check_freshness(1800000000, 1800000031)

    def test_clock_uses_only_configured_ca_with_partial_chain(self):
        context = mock.Mock()
        context.verify_flags = 0
        handler = mock.Mock()
        opener = mock.MagicMock()
        response = mock.MagicMock()
        response.status = 200
        response.headers = Message()
        response.headers.add_header("Date", "Fri, 15 Jan 2027 08:00:00 GMT")
        opener.open.return_value.__enter__.return_value = response
        with mock.patch.object(meeting_provision.ssl, "SSLContext", return_value=context) as create, \
             mock.patch.object(meeting_provision.urllib.request, "HTTPSHandler", return_value=handler) as https, \
             mock.patch.object(meeting_provision.urllib.request, "build_opener", return_value=opener):
            meeting_provision.check_server_clock("https://example.com", "configured-ca", wall=lambda: 1800000000, monotonic=lambda: 1)
        create.assert_called_once_with(meeting_provision.ssl.PROTOCOL_TLS_CLIENT)
        context.load_verify_locations.assert_called_once_with(cadata="configured-ca")
        self.assertTrue(context.verify_flags & meeting_provision.ssl.VERIFY_X509_PARTIAL_CHAIN)
        https.assert_called_once_with(context=context)

    def test_server_clock_requires_unambiguous_matching_date(self):
        class Response:
            status = 200
            def __init__(self, dates):
                self.headers = Message()
                for date in dates:
                    self.headers.add_header("Date", date)
            def __enter__(self): return self
            def __exit__(self, *args): return False
        class Opener:
            def __init__(self, dates): self.dates = dates
            def open(self, request, timeout):
                self_request = request
                assert self_request.full_url == "https://example.com/health" and timeout == 5
                return Response(self.dates)
        good = "Fri, 15 Jan 2027 08:00:00 GMT"  # 1800000000
        meeting_provision.check_server_clock("https://example.com", "unused", Opener([good]),
                                            wall=lambda: 1800000000, monotonic=lambda: 1)
        for dates in ([], [good, good], ["Fri, 15 Jan 2027 08:01:00 GMT"]):
            with self.assertRaises(ValueError):
                meeting_provision.check_server_clock("https://example.com", "unused", Opener(dates),
                                                    wall=lambda: 1800000000, monotonic=lambda: 1)
        with self.assertRaises(ValueError):
            meeting_provision.check_server_clock("http://example.com", "unused", Opener([good]))


if __name__ == "__main__":
    unittest.main()
