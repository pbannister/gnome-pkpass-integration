# Record: the knowns and unknowns document

## Outcome

`documents/10-pkpass-knowns-and-unknowns.md` states how far each statement in
the format reference can be trusted. The format reference says what the format
is; this document says what is documented, what was observed, what is only
convention, what is unknown, and what cannot be known from this machine.

The work is in commit `21dd882`.

- Every claim carries one of five labels: **Documented**, **Observed**,
  **Convention**, **Choice**, or **Unknown**.
- The document is dated (2026-10-10) and says why: the format has no standard,
  Apple's documentation moves, and an undated claim about it becomes a false
  statement nobody notices.
- It is listed in `documents/README.md`, pointed at from the format reference,
  and named in the project README's research findings.

## Decisions

- **A separate document rather than a section of the format reference.** The
  reference is a description of the format and is read by anyone wanting a field
  table; the epistemic split is read by someone deciding whether to trust a
  statement. Mixing a confidence label into every line of the reference would
  make both harder to read, and the reference is the wider document.
- **Five labels, not a confidence percentage.** "Documented", "observed",
  "convention", "choice", and "unknown" are distinguishable by how they could be
  settled: a document could change, a measurement could be repeated, an
  implementation could diverge, a decision could be revisited, and an unknown
  could need hardware nobody here has. A number would hide which of those
  applies.
- **The `Choice` section exists because the format is silent.** Where there is
  no rule, the project's behaviour is a decision, and a decision recorded only
  in code is one nobody can review. Four-digit hex colour order, barcode
  selection, and the UTF-8 assumption for `messageEncoding` are all in it.
- **"Not knowable from here" is separated from "unknown".** Not knowing what
  Wallet enforces is a different problem from not having a device to ask. The
  first could be answered by a document; the second cannot be answered at all in
  this environment, and conflating them would suggest the project is closer to
  certainty than it is.
- **No real pass value appears.** The document describes what a real pass
  demonstrated — mixed compression, a manifest-first archive, the CMS digest
  differing from the manifest's — without carrying any of its data. The
  repository's synthetic fixture is the only example material.

## Verification

- `grep -c` for the five labels reports 23 occurrences in 332 lines, so the
  labelling is applied throughout rather than announced and forgotten.
- The privacy scan for the real pass's identifiers over the new file reports no
  match, on 2026-10-10.
- `make test` — 25 tests, all pass, on 2026-10-10, including
  `09-prompt-contract` and `08-skeleton-diff`.
- `make site` — the page set builds and `scripts/leak-gate.sh site.out` reports
  `clean`, on 2026-10-10.
- `scripts/skeleton-diff.sh` from the skeleton — 0 drift; `documents/README.md`
  is reported as appended, which is the sanctioned way to add to a shared file.

## Commits

- `21dd882` — docs: what is known and what is not known about the format. The
  document, its task, and the three index entries.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the work commit above rather than itself.
