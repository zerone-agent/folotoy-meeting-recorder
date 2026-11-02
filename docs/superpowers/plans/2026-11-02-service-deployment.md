# Service Deployment Implementation Plan

**Goal:** Package and verify a single-instance meeting service with traceable releases.

**Architecture:** A non-root Node image runs the existing HTTPS service with its complete SQLite directory on a persistent bind mount. CI tests and builds; a manual release workflow publishes an immutable digest only after verifying version and prior tags.

**Tech Stack:** Node 22, Docker, Compose, GitHub Actions, GHCR.

**Spec:** docs/2026-10-30-service-deployment-design.md

- [ ] Add container image and Compose configuration with private data and read-only certificate mounts.
- [ ] Add CI and version release workflow; reject duplicate remote tags and report digest and commit.
- [ ] Document provisioning, secure configuration, backup, upgrades and fail-closed recovery.
- [ ] Run service tests, Compose configuration and container lifecycle tests when Docker is available; report missing evidence.
