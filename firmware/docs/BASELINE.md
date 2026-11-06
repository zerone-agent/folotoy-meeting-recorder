[简体中文](BASELINE.zh_CN.md)

# Firmware baseline

> Historical baseline: the status below describes an earlier development stage. For current integration and bounded device results, see [device tests](../../research/2026-11-03-agent-commissioning-upload-test.md).


The source is copied from ai-passport-soundcore commit `569b863c4f5444bf5f5c914ceb2ba6bec2bf94ad`, preserving its license and documentation. Build output and downloaded dependencies are excluded.

Mount failure does not automatically format the recording partition. Automatic upload, Wi-Fi provisioning, and cleanup after server confirmation are not yet wired together; this is not a finished meeting recorder. An empty partition requires an explicit initialization procedure, and mount failure must not erase data.

Meeting deletion and credential revocation state machines are not implemented. Legacy remote delete and factory reset commands currently fail to avoid erasing recordings or resetting credentials without user confirmation. This changes reference firmware compatibility and does not replace the planned manual deletion and revocation flows.
