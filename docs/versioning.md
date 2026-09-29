# Versioning

## Feature Builds

Feature branches produce a version string of the form `<branch-safe>-<sha7>`.

The branch name is lowercased and non-alphanumeric characters are replaced with hyphens. The SHA is truncated to 7 characters.

Example: branch `feature/add-stats` at commit `abc1234def` produces version `feature-add-stats-abc1234`.

Script: `scripts/compute_feature_version.sh`

## Master Releases

Master releases use semantic versioning (`MAJOR.MINOR.PATCH`), bumped from the latest `v*` tag.

The bump type is determined by the merged PR title prefix:

| Prefix | Bump |
|---|---|
| `major` | Major (x.0.0) |
| `minor` | Minor (0.x.0) |
| `fix`, `patch`, `path` | Patch (0.0.x) |

If no `v*` tags exist, the base version is `0.0.0`.

Script: `scripts/compute_master_semver.sh`

## Embedded App Version

The CMake option `TRACKER_APP_VERSION` is compiled into the binary via `AppVersion.hpp.in`. CI sets it to the computed version; local builds default to `${PROJECT_VERSION} (localbuild)`.
