# Record: GNOME PKPASS Integration — project start

## Outcome

`gnome-pkpass-integration` exists as a project derived from
`00-project-skeleton` and following `gnome-appimage-integration` as the worked
example of GNOME desktop integration.
It reads Apple Wallet `.pkpass` files, checks them, draws them, and registers
itself as the application that opens the two Wallet MIME types.

The work is in commit `6193f6d`.

What exists:

- `sources/crypto/sha1.cpp` — SHA-1, the digest a pass manifest records.
- `sources/json/` — the minimal JSON value and parser.
- `sources/zip/` — a ZIP reader that honours each member's own compression
  method and checks the CRC.
- `sources/pkpass/` — the pass model, the reader, and the manifest and
  signature verification.
- `sources/render/` — the SVG card renderer.
- `sources/integration/` — the MIME definition and handler-database integration.
- `sources/tools/` — `pkpass-inspect`, `pkpass-render`, `pkpass-integrate`, and
  the optional GTK4 `pkpass-viewer`, with the handler icon.
- Features `03` through `06`, tasks `02` through `05`, and tests `14`, `15`,
  `30`, `31`, `40`, `60`, and `90`.
- `documents/08-pkpass-format.md` and `documents/09-mime-type-association.md`.

Features `01` and `02`, the skeleton's 00–13 tests, and the shared rule corpus
are kept as they are.

`make test` runs 21 tests and all pass.
`make site` builds the page set and the sanitization gate passes over it.

## Decisions

- **The real pass is not committed.** A `.pkpass` carries its holder's name, an
  identifier, and a barcode payload. The example that prompted the project was
  read to learn the format and is not in the repository. The fixture at
  `dataflow.in/pkpass/sample/` is synthetic: its values are invented, including
  its coordinates, which were first copied from the example and were replaced
  when a review caught them. The repository scan for the example's identifiers
  is clean.
- **The project writes its own MIME definition at the user level even where
  shared-mime-info already ships the type.** A per-user definition outranks the
  system one and is the only way `generic-icon` can attach this project's icon
  to `*.pkpass`. A definition the project did not write is backed up and
  restored byte for byte, so a distribution's own definition is never lost.
- **Two types are registered.** `application/vnd.apple.pkpass` for a pass and
  `application/vnd.apple.pkpasses` for a bundle, because Apple names the second
  and shared-mime-info has shipped the first since 2.2 and the second since 2.5.
- **`pkpass-inspect` exits zero on a mismatched manifest.** A mismatch is a
  finding about the file, reported in words and in `verification.manifestOk`; a
  nonzero exit is reserved for a file that cannot be read, so a caller can tell
  a wrong pass from a non-pass.
- **The barcode is not drawn as a symbol.** The project has no QR or PDF417
  encoder, and a symbol that might not scan is worse than a payload a person can
  read and copy. The card shows the format, the alt text, and the payload; the
  limitation is `PASS-RENDERER-R009`, `documents/08-pkpass-format.md`, and an
  open `TODO.md` item.
- **The GTK4 viewer is built only where the GTK4 development files are
  present.** This host has none, so the target is skipped here; the handler
  falls back to printing the equivalent commands. The viewer's source is
  therefore **not compiled or exercised by this work's verification** — see
  Verification below.

## Verification

- `make test` — 21 tests, all pass, on 2026-10-10. The run covers the SHA-1
  vectors, the JSON reader including its refusals, the ZIP reader over a
  fixture that mixes stored and deflated members, the pass reader, the renderer,
  the command-line tools and their exit codes, and a full handler install and
  uninstall inside a throwaway `HOME` with `xdg-mime`.
- `make site` — the page set builds and `scripts/leak-gate.sh site.out` reports
  `clean`, on 2026-10-10.
- The reader was run against the real example pass during development: its
  manifest's seven SHA-1 digests all matched, its nine members read as stored
  and deflated, and its colours parsed from the `rgb(86, 39, 126)` form with
  spaces. That run is what the synthetic fixture is modelled on. It is not
  repeatable from the repository, because the example is deliberately not
  committed.
- **Not verified:** `pkpass-viewer` was not compiled or run, because the GTK4
  development files are absent on this host. Its correctness rests on review
  only, and it must be built and exercised on a host with GTK4 before it is
  trusted. This is the honest state of that target, not a passing result.

## Commits

- `6193f6d` — feat: read, render, and integrate Apple Wallet pkpass files. The
  work: sources, tests, features, tasks, documents, the fixture, the build and
  install scripts, and the project's own README, TODO, and PHASES.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the work commit above rather than itself.

## Corrections

- 2026-10-10 — The final drift check against the skeleton found two shared-file
  violations (an extended `tests/README.md`, which is byte-checked, and a
  mid-file edit to `documents/README.md`, which is append-only) and a Makefile
  missing the `status`, `maturity`, and `all` targets that `AGENTS.md` and
  `scripts/prompt-maturity.sh` name. Commit `87b7807` restores the two shared
  files, moves the project's test notes into this project's `README.md`, lists
  the new documents in an appended project section, and restores the targets.
  `scripts/skeleton-diff.sh` then reports 0 drift, and `make status` and
  `make maturity` work.

