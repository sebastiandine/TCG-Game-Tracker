# CI/CD Guide

This document describes the CI/CD setup for TCG Game Tracker.

## Workflow Topology

Two orchestrator workflows trigger from pushes:

- **`feature-ci.yml`** — runs on every push to a non-`master` branch. Triggers SonarQube scan, Linux build, and Windows build.
- **`master-ci.yml`** — runs on pushes to `master` (merged PRs). Computes a semantic version, triggers SonarQube scan and Windows build, then creates a GitHub release.

Each orchestrator calls reusable workflows for OS-specific work:

- `feature-linux.yml` — Linux build + test + package
- `feature-windows.yml` — Windows build + test + NSIS installer + package
- `master-windows.yml` — Windows build + test + NSIS installer for release

## PR Title Guard

**`master-pr-title-guard.yml`** validates that PR titles targeting `master` start with a semantic prefix: `major`, `minor`, `fix`, `patch`, or `path`. This prefix drives the version bump in the release pipeline.

## Versioning

- Feature builds: `<branch>-<sha7>` (from `scripts/compute_feature_version.sh`)
- Master releases: semantic version bumped from the latest `v*` tag (from `scripts/compute_master_semver.sh`)

See [versioning.md](versioning.md) for full details.

## Artifacts

| Artifact | Format |
|---|---|
| `tcg-game-tracker-linux-<version>.zip` | Linux portable bundle |
| `tcg-game-tracker-windows-<version>.zip` | Windows portable bundle |
| `tcg-game-tracker-windows-installer-<version>` | NSIS installer |

## Release Flow

1. Merge a PR into `master` with a valid title prefix.
2. `master-ci.yml` resolves the version, builds, and creates a GitHub release with the installer and zip.
