# AGENTS.md

Long-form contributor documentation that lives outside the source tree.

## Files

- `README.md` — index page that clusters docs by area and links to all documents in this directory.
- `intro-to-new-developers.md` — onboarding map for new contributors.
- `dow-doc-build-locally.md` — complete local build/setup reference for Windows and Linux.
- `testing-and-test-code-of-conduct.md` — testing workflow plus expected standards for writing and maintaining tests.
- `ci-cd-guide.md` — CI workflows, merge guards, artifacts, and release execution.
- `versioning.md` — feature-build version format, semantic release rules on `master`, and tag/app-version behavior.
- `windows-installer.md` — how the NSIS-based Windows installer is built, configured, and versioned.
- `database/README.md` — database approach, file location, and how to add a new table.
- `database/schema.md` — all table definitions, columns, constraints, and relations.

## Conventions

- These docs are reference material for contributors, not user-facing release notes. Keep them precise.
- Code examples should be C++20 and quote real file paths from the repo.

## Required follow-ups

- After adding a new file under `docs/` you **must** add it to the file list above **and** to `README.md` so the index stays complete.
- Do **not** rename, move, or split this file without first updating every other `AGENTS.md` that points at it.

## Anti-patterns

- Don't link to external repos as the source of truth — the C++ code under `core/`, `ui_wx/`, and `app/` is the spec.
- Don't duplicate the per-package `AGENTS.md` content here; cross-link instead.
