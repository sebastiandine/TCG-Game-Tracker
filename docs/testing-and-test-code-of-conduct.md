# Testing Guide And Test Code Of Conduct

This guide defines how testing works in TCG Game Tracker and which standards test code must meet.

## Testing Focus

The project prioritizes deterministic, fast, behavior-oriented testing. Most automated coverage targets `core/` logic and infrastructure/service behavior, while UI validation remains manual.

## Test Setup

- framework: `doctest`
- primary target: `tracker_core_tests`
- location: `tests/`
- default behavior: tests enabled via `TRACKER_BUILD_TESTS=ON`

## Run Tests

From repository root:
```bash
cmake -S . -B build -DTRACKER_BUILD_TESTS=ON
cmake --build build --target tracker_core_tests
ctest --test-dir build --output-on-failure
```

## Coverage Surface

The CI Sonar scan reports coverage against `core/` paths that `tracker_core_tests` can execute. `ui_wx/` and the `app/` composition root are excluded from Sonar's coverage calculation because they are not run under the doctest suite; UI behavior is covered by manual validation.

## Manual UI Validation

Because there is no UI automation, UI-affecting changes require manual checks:

- theme switching (dark and light)
- dialog and popup behavior
- settings save and load

For theming work, rebuild and run the final app target (`tracker`) instead of validating only static library targets.

## Test Code Of Conduct

### Keep Tests Hermetic

- do not call real network services
- do not depend on local machine files
- use fakes and in-memory adapters where possible

### Test Behavior, Not Internals

- assert externally visible outcomes
- avoid brittle assertions tied to incidental implementation details
- prefer domain-level expectations over call-level trivia

### Keep Tests Deterministic

- no unseeded randomness
- no timing-sensitive assertions that can flap
- no ordering assumptions unless ordering is part of the contract

### Keep Tests Readable

- one intent per test case
- descriptive test names
- clear arrange/act/assert flow
- minimal abstraction for small tests

### Keep Tests Maintainable

- update tests when contracts change — don't leave broken or skipped tests behind
- remove tests that no longer reflect the codebase
- add tests for new behavior in the same commit as the behavior
