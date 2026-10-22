# Record: Adopt the shared rules

## Outcome

This project now pins the shared rules at `2026-10-22.13`. `rules.lock` holds the
version and a SHA-256 per vendored file, `scripts/rules-check.sh` verifies the
copy, and `tests/15-rules-pin.sh` runs it in the suite.

The adoption was `breaking`: the contract gains a non-overridable safety
precedence level, modification becomes authorization-based, and nine capability
conventions apply.

The validator found nothing. Every finding came from the two hazards this
project's lineage carries, both known before the adoption started.

## Decisions

- **The test helper was split before it could be lost.** It had 72 lines with
  `build_program`, `build_synthetic_pkpass`, and the repository, build, and
  fixture paths. Those moved into `tests/lib/project_helpers.sh` (43 lines), which
  the vendored helper sources. This is the extension point from `2026-10-22.13`,
  and the second project to use it since its sibling lost nineteen tests without
  it.
- **Three local files carried the pre-rename path.**
  `prompts/common/02-universal-rules.md` is now `01-universal-rules.md`, and this
  project still named the old one in the glossary, in `scripts/site-condense.sh`
  (comment only), and in `scripts/skeleton-diff.sh` (its file list, which its own
  self-test checks). All three updated. This is the second project to need the
  `skeleton-diff` edit; the script is superseded by `rules-check`, and every
  skeleton-lineage project will owe the same repair until it is retired.
- **`AGENTS.md` gained a Shared Rules section.** It existed before the rules were
  pinned and said nothing about them.
- **`PHASES.md` is declared** in `prompts/10-project.md`.
- **The pin test is `15-rules-pin.sh`.** The `00`–`14` band is fully used by this
  project and the vendored tests, so it took `15`, the next free number.

## Verification

- `rules-conform`: nothing to conform.
- `rules-check`: ok (v2026-10-22.13, 39 files, 0 deviations).
- `make test`: 27 test scripts, all passed.

## Behavioral evidence

**Not run.** All six eval cases are runnable here — this project carries both
`scripts/status.sh` and `scripts/site-state-fetch.sh` — as they are in
`gnome-vcard-integration`. Two projects can now produce a complete behavioral
result, and neither has. That is the gap recorded in
`00-project-rules/documents/eval-portability.md`, and it is the largest open item
in the system: every adoption since the first has been mechanically verified and
behaviorally unverified.

## What this adoption cost the library

Nothing: it used the extension point from `.13` and local edits only.

## Commits

- Adoption commit — the vendored rules, the split helper, and this record.
