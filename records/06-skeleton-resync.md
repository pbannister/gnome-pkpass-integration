# Record: re-sync with the project skeleton

## Outcome

The project is back in step with `00-project-skeleton` after the skeleton's
capability split and context-budget work. `scripts/skeleton-diff.sh` reports
**0 drift, 0 missing, 2 appended, and 2 reference adaptations** — the healthy
state for a derived project.

The work is in commit `d09a3f3`.

What changed:

- **Rule files taken as they are**: `prompts/01-contract.md`,
  `prompts/02-workflow.md`, `prompts/03-conventions.md`, `prompts/README.md`,
  `tests/README.md`, and the five new `prompts/capabilities/` conventions
  (`01-generated-documentation`, `02-version-information`,
  `03-generated-data-products`, `04-release-and-publishing`,
  `05-pinned-dependencies`), which split the capability rules out of the
  conventions so they load only for a task that touches one.
- **Append-only files re-based**: `documents/README.md` and
  `prompts/common/03-glossary.md` now carry the skeleton's new text with this
  project's section appended, rather than a hand merge.
- **Scripts updated**: `skeleton-diff.sh`, `context-budget.sh` (new),
  `install.sh`, `release-package.sh`, `prompt-maturity.sh`, `version-generate.sh`.
- **Tests updated**: `09-prompt-contract.sh`, `10-prompt-validator.sh`,
  `14-context-budget.sh` (new).
- **Documents updated**: `04-lessons-from-homelab.md` (the generated-documents
  reference moved to the capability file), `07-prompt-maturity.md` (two
  dimensions now score full marks because they acquired evidence).
- **New evals**: `04-prompt-injection.md`, `05-acceptance-remains-human.md`.
- **Removed**: `prompts/common/00-overview.md` and `01-requirements.md`.
- **Added**: the `context` target, and `.aider.conf.yml` minus the two files the
  baseline no longer loads.

`make test` is 26 tests, all passing; `make context` measures 39 477 of the
declared 41 984 bytes; the prompt-maturity score rose from 26 to 28 of 30.

## Decisions

- **The skeleton's rule files win, without exception.** They are byte-identical
  by contract, so the only question was what to do about the project's own text.
  For the two append-only files that was re-basing; for the rule files there was
  nothing to preserve, because this project had never edited them.
- **The project's two real adaptations were re-applied, not merged.** Both are
  one-line substitutions against the skeleton's copy — the page title in
  `site-build.sh` and the fixture number in `tests/10` — so re-applying them to
  the skeleton's new file is safer than a three-way merge of a script.
- **Two files were deleted, not kept.** The skeleton folded
  `prompts/common/00-overview.md` and `01-requirements.md` into the contract and
  the common rules, and the new validator refuses a universal rule file the
  registry does not name. Keeping them would have meant carrying two orphan
  rule files that the corpus no longer recognises.
- **The reader tests were renumbered rather than the new test.** The skeleton
  names `tests/14-context-budget.sh` in the shared `tests/README.md`, and that
  file is byte-identical by contract — a renamed copy would leave the shared
  README naming a file that does not exist. So this project's SHA-1 and JSON
  reader tests moved to `16` and `17`, which keeps them in the readers band and
  leaves the skeleton's name truthful.
- **The absolute-home-path fixture moved file.** This project ignores `.vscode/`
  (an owner decision), so a path written into `.vscode/settings.json` is not
  tracked and the validator rightly skips it — the fixture was passing for the
  wrong reason (it was being accepted, and the test caught that). The fixture
  now writes into `.aider.conf.yml`, which is tracked and covered by the same
  check. That is why an owner-level `.gitignore` choice surfaced as a validator
  failure: the check keys on tracked tool configuration.
- **`make context` is the new gate.** The always-loaded context is now a
  measured quantity with a declared budget, so a rule added to the baseline has
  to fit. It sits alongside `make maturity` as a self-assessment target.

## Verification

- `scripts/skeleton-diff.sh` from the skeleton — 0 drift, 0 missing, 2 appended
  (both sanctioned), 2 reference artifacts adapted (both sanctioned), on
  2026-10-11.
- `make test` — 26 tests, all pass, on 2026-10-11, including the new
  `14-context-budget`, the rebuilt validator, and its adversarial corpus
  (55 rejected, 6 accepted).
- `make context` — 39 477 of 41 984 bytes, 5 baseline files plus the entry
  point, on 2026-10-11.
- `make site` — the page set builds and `scripts/leak-gate.sh site.out` reports
  `clean`; the index title is still `GNOME PKPASS Integration`, so the project's
  `site-build.sh` adaptation survived the copy.
- `make maturity` — 28/30, up from 26/30; the remaining ceilings are behavioral
  evals and parallel work.

## Commits

- `d09a3f3` — chore: re-sync with the project skeleton. The rule files, the
  scripts, the tests, the documents, the evals, the two deletions, the
  renumbering, the `context` target, and the README and TODO updates.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the work commit above rather than itself.

## Left open

- The re-synced pages are not yet republished. `make site` here is current, but
  the homelab deploy has not been re-run since this change, so the live pages
  still show the pre-re-sync corpus — including the two rule files this commit
  deletes.
- The skeleton's `tests/14-context-budget.sh` sits in the `10`–`19` band that the
  shared `tests/README.md` reserves for readers and parsers, while it tests the
  prompt corpus. The inconsistency is the skeleton's, and this project inherited
  it deliberately: the shared README names the file, so the number follows the
  name. Worth raising upstream rather than working around here.
