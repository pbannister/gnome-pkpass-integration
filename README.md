# GNOME PKPASS Integration

This project handles Apple Wallet `.pkpass` files on the GNOME desktop.

A `.pkpass` is a ZIP built by an issuer: `pass.json` describes the pass,
`manifest.json` records a SHA-1 digest per member, and `signature` carries a
detached PKCS#7 signature over the manifest.
The format has no Linux viewer in a stock desktop, so a double-clicked pass has
nothing to open it.

This project reads a pass, checks it, draws it, and registers itself as the
application that opens the two Wallet MIME types.

- `pkpass-inspect` reads a pass and prints what it contains, including whether
  the manifest digests still match and whether the signature checks out.
- `pkpass-render` draws the pass as a standalone SVG card, carrying its colours,
  its logo, and a scannable QR symbol of the barcode, with the payload
  re-indented and its URLs clickable.
- `pkpass-integrate handler install` associates `application/vnd.apple.pkpass`
  and `application/vnd.apple.pkpasses` with this project, gives the file type
  its icon, and remembers what opened those types before, so the change can be
  reversed.

Everything the desktop does is also a command line.

**Status: phase 4 — the reader, the renderer, the handler, and the tools are
complete and `make test` passes; release `v2026-10-23-master` is published at
<https://github.com/pbannister/gnome-pkpass-integration/releases/latest>, and
the page set is still to be published.**
The barcode is drawn as a real QR symbol, encoded from the message bytes at
error correction level M; the payload below it is re-indented when it is JSON,
and an `http` or `https` URL in a field or in the payload is clickable. The
GTK4 viewer is built and exercised here under Xvfb, and is skipped where the
GTK4 development files are absent.

## Installing

A release can be installed with one command:

```sh
curl -fsSL https://raw.githubusercontent.com/pbannister/gnome-pkpass-integration/master/scripts/install.sh | sh
```

The script fetches the release tarball for this machine, checks it against the
release's `SHA256SUMS`, installs into `$HOME/.local`, and registers the handler.
`PREFIX` installs elsewhere, `VERSION` pins a release tag, and where no prebuilt
build exists for the machine it builds from the source archive instead.

`make install` installs the tools into `$HOME/.local` (`PREFIX` overrides) and
then registers the handler, so a downloaded pass opens as soon as the install is
done.

Registration changes what opens a file type, so it is skippable and reversible:

```sh
PKPASS_INTEGRATION_NO_HANDLER=1 make install   # install the tools, register nothing
pkpass-integrate handler status                # show what each type opens, and what this project installed
pkpass-integrate handler uninstall             # restore what opened them before
```

A `make install` that cannot register the handler still installs the tools; it
prints the reason and exits nonzero, so a scripted install never reports success
with the association missing. The release installer reports a failed
registration and leaves the tools in place.

The install writes only under `$XDG_DATA_HOME`, `$XDG_CONFIG_HOME`, and the
temporary directory, and never needs root.

## Commands

| Command | Purpose |
| ------- | ------- |
| `pkpass-inspect FILE.pkpass` | report the container, the pass, the fields, the barcodes, the locations, the images, the manifest result, and the signature |
| `pkpass-inspect --json FILE.pkpass` | print the same facts as one JSON document |
| `pkpass-inspect --signature FILE.pkpass` | also check the PKCS#7 signature and name the signer |
| `pkpass-render FILE.pkpass` | write the front of the pass as SVG to standard output |
| `pkpass-render --back FILE.pkpass` | draw the reverse, where the back fields are |
| `pkpass-render --no-images FILE.pkpass` | draw the card without the embedded logo |
| `pkpass-render --output FILE FILE.pkpass` | write the SVG to a file |
| `pkpass-render --qr-png FILE FILE.pkpass` | also write the barcode as a PNG, to scan from another screen |
| `pkpass-integrate handler status` | report each type's default, the entry, the definition, the type icons, and the record |
| `pkpass-integrate handler install` | write the MIME definition, the handler entry, the three icons, and the record, and set both types to this handler |
| `pkpass-integrate handler uninstall` | restore the recorded previous handlers and remove what this project installed |
| `pkpass-integrate handle FILE.pkpass` | the desktop entry point: open the pass in the viewer, or print the equivalent commands |
| `pkpass-viewer FILE.pkpass` | the GTK4 window (built only where GTK4 is present) |
| `pkpass-inspect --version` | print the build-time version |

`pkpass-inspect` exits zero on a pass whose manifest does not match: the mismatch
is a finding about the file, and the report says so in words and in
`verification.manifestOk`. A nonzero exit means the file could not be read at
all, so a script can tell "this pass is wrong" from "this is not a pass".

## What the handler writes

| Path under `$XDG_DATA_HOME` | What it is |
| --------------------------- | ---------- |
| `mime/packages/pkpass.xml` | the definition of both Wallet types, with their globs, magic, and this project's icon |
| `applications/pkpass-handler.desktop` | the handler entry, whose `Exec` is `<tool> handle %f` |
| `icons/hicolor/scalable/apps/pkpass-handler.svg` | the handler's icon |
| `icons/hicolor/scalable/mimetypes/application-vnd.apple.pkpass.svg` | the file icon for a pass |
| `icons/hicolor/scalable/mimetypes/application-vnd.apple.pkpasses.svg` | the file icon for a bundle |
| `gnome-pkpass-integration/pkpass-handler.manifest` | the record: what was written, what was displaced, and what opened each type before |

The record is what makes an uninstall possible; without it, `handler uninstall`
refuses rather than guessing.
A definition at `mime/packages/pkpass.xml` that this project did not write is
backed up first and restored byte for byte, because a distribution may already
define the type and its definition is not this project's to lose.

## Top-Level Map

- `README.md` is the project overview.
- `AGENTS.md` is the coding-agent entry point; it points at `prompts/` and defines no rule.
- `TODO.md` tracks pending and completed project tasks.
- `PHASES.md` records the current phase and its milestones.
- `prompts/` contains LLM interaction rules, common requirements, feature requirements, task definitions, episode work orders, and the capability conventions (`prompts/capabilities/`) that are loaded only when a task touches one.
- `documents/` contains human-consumption documents: the interaction pattern, worked examples, and the format research.
- `records/` contains version-controlled outcome, incident, and handoff records.
- `tools/` contains tool-specific rules.
- `sources/` contains the C++ readers, the renderer, the integration, and the command-line tools.
- `scripts/` contains project scripts.
- `tests/` contains tests and validation code.
- `dataflow.in/` contains input data, including the synthetic pass fixture.
- `dataflow.out/` contains generated data output and the build tree (not version-controlled).
- `logs/` contains generated logs (not version-controlled).
- `site.in/` contains static-site input.
- `site.out/` contains generated static-site output (not version-controlled).
- `Makefile` drives the build (`make build`), the tests (`make test`), the pages (`make site`), the install (`make install`), and cleanup (`make clean`).

## Building and Testing

- `make build` configures and compiles the readers, the renderer, the integration, and the tools into `dataflow.out/build/`.
- `make install` installs `pkpass-inspect`, `pkpass-render`, and `pkpass-integrate` (and `pkpass-viewer` where GTK4 is present) into `$HOME/.local/bin` (`PREFIX` overrides), with the icon, and registers the handler; `PKPASS_INTEGRATION_NO_HANDLER=1` skips the registration.
- `make test` runs every test in `tests/` and writes a timestamped log to `logs/`.
- `make status` reports the phase, the next open TODO item, the last test run, and the working-tree state in one screen.
- `make context` measures the always-loaded context (the entry point plus the mandatory baseline) against the budget `prompts/02-workflow.md` declares.
- `make maturity` scores the machine-checkable subset of the prompt-maturity rubric.
- `make site` builds the published page set, then runs the sanitization gate over `site.out/` (`make check` runs the gate alone).
- `make release` builds, stages the programs into `dataflow.out/build/bin/`, checks that the version names the current commit, and writes the release assets into `dataflow.out/release/`.
- `make release-publish` builds from the commit being released, tags it, and creates or updates the GitHub release with the tarball and its checksum; it needs the GitHub CLI. Without `TAG`, the tag is `v<date>-<branch>` (for example `v2026-10-23-master`); a tag already on the commit is reused, and an explicit `TAG` overrides it.
- `make clean` removes generated output.

The C++ build uses CMake with the highest warning level and treats warnings as
errors.

The tests follow the skeleton's reserved bands: `14` for the context budget,
`16`–`17` for the SHA-1 and JSON readers, `30`–`31` for the ZIP and pass
readers, `40`–`42` for the renderer and the QR encoder, `60` for the
command-line tools, and `90`–`96` for the handler install and uninstall, the
viewer window, and the barcode decode.
The C++ unit tests (`tests/*_test.cpp`, `tests/test_harness.h`) are built by
`sources/CMakeLists.txt` and run by a shell test, which also builds the fixture;
`tests/lib/test_helpers.sh` carries `build_program` and `build_synthetic_pkpass`.
The fixture is synthetic on purpose: a real pass belongs to a person, so the
format-shaped sample under `dataflow.in/pkpass/sample/` gives the tests a pass
that is structurally real and nobody's.
The handler test points `HOME` and the XDG paths at a throwaway directory, so it
exercises `xdg-mime`, `update-mime-database`, and the icon cache without
touching this machine's own desktop.

## Source Layout

- `sources/crypto/` contains SHA-1, which is the digest a pass manifest records.
- `sources/json/` contains the minimal JSON value and parser.
- `sources/zip/` contains the ZIP reader, which honours each member's own compression method.
- `sources/pkpass/` contains the pass model, the reader, and the manifest and signature verification.
- `sources/qr/` contains the QR encoder the barcode is drawn with.
- `sources/render/` contains the SVG card renderer.
- `sources/integration/` contains the MIME-definition and handler-database integration.
- `sources/util/` contains file and process helpers.
- `sources/tools/` contains the command-line front ends, the GTK4 viewer, and the icon.
- `sources/version/` contains the build-time version reporting.
- `sources/CMakeLists.txt` defines the targets; the build tree is written to `dataflow.out/build/`.

The graphical viewer is C++ with GTK4.
The build needs the GTK4 development files (`libgtk-4-dev` for Debian and
Ubuntu, `gtk4-devel` for Fedora); without them the viewer target is skipped and
`pkpass-integrate handle` prints the equivalent command-line commands instead.
`tests/95-viewer.sh` opens the window on a private Xvfb display and checks the
window's title and WM class, so the viewer is covered by `make test` wherever
GTK4 and Xvfb are present, and skips cleanly where they are not.
Nothing in the viewer needs `librsvg`: the logo is a PNG shown through
`GtkPicture`, and the only SVG the project writes is the rendered card file.

## Research Findings

The format research the readers implement is recorded in `documents/`.

- `documents/08-pkpass-format.md` — the `.pkpass` container, `pass.json`, the field model, the colours, the barcode formats, localization, and what this project does and does not yet do with them.
- `documents/09-mime-type-association.md` — how the desktop resolves a file type to an application, and exactly what this project writes to become the handler for the Wallet types.
- `documents/10-pkpass-knowns-and-unknowns.md` — what is documented, observed, and only conventional about the format, what is unknown, and what cannot be known without Apple hardware.

## For Human Contributors

Human contributors should begin by reading:

- `prompts/README.md`
- `documents/README.md`

The prompt corpus is pinned, not copied: `rules.lock` records the
`00-project-rules` version and a digest per vendored file, and
`scripts/rules-check.sh` fails when one is edited. Customise by extending — a
project-specific rule goes in a local prompt file numbered after the shared
ones — rather than by editing shared text, which the check treats as drift.
`tests/09-prompt-contract.sh` validates the corpus itself.

## For the LLM

The mandatory baseline is defined in `prompts/02-workflow.md` section 1, and its
size is measured by `make context` against the budget that same section
declares. It is, in order:

1. `prompts/01-contract.md`
2. `prompts/02-workflow.md`
3. `prompts/03-conventions.md`
4. `prompts/common/02-universal-rules.md`
5. `prompts/flavors/01-semantic-sort-naming.md`
6. `AGENTS.md` (the entry point, read first)

- These define the interaction rules, the workflow, the formatting conventions, and the global rules.
- The LLM must follow the workflow defined in `prompts/02-workflow.md` for every task.
- A capability convention under `prompts/capabilities/` is loaded only for a task that touches that capability, so it stays out of the always-loaded context.
- This project targets C++, so `prompts/flavors/02-cpp-conventions.md` applies to source work.
- `prompts/common/03-glossary.md` is a reference: load it when a term's meaning decides the work.

## Project Scope

The project reads one file format and integrates it into the desktop.

1. Read a `.pkpass` container: the ZIP members, each with its own compression method.
2. Read `pass.json`: the top-level keys, the pass style, the five field groups, the colours, the barcodes, and the relevance locations.
3. Check the pass: the SHA-1 digests in `manifest.json`, and the detached PKCS#7 signature over the manifest.
4. Draw the pass as a standalone SVG card, with a scannable QR symbol of the barcode and its payload, JSON re-indented and URLs clickable.
5. Register `application/vnd.apple.pkpass` and `application/vnd.apple.pkpasses` with the desktop, and reverse that registration from a record.

The project never modifies a pass, and it never requires root.
It reads a pass's contents in order to display them; it does not transmit them
anywhere.

## Project Settings

- Automatic task commits: enabled (see `prompts/02-workflow.md` §7.1).
- Privacy boundary: a real `.pkpass` belongs to a person and carries a name, an
  identifier, and a barcode payload. No real pass, and no value taken from one,
  is committed to this repository; the fixture at
  `dataflow.in/pkpass/sample/` is synthetic and exists so the tests have a pass
  that is structurally real and nobody's.
- Live-state tests: none. Every test runs against fixtures and a throwaway
  `HOME`, so `make test` touches neither the real desktop nor the network.

## Project Pages (publishing conventions)

- A project derived from the skeleton publishes the standard page set
  (status, dashboard, condensed todo/prompts/documents) per
  `prompts/features/02-project-pages.md` and
  `documents/06-project-pages.md`.
- `make site` builds the full set (`scripts/site-build.sh` +
  `scripts/site-condense.sh`).
- Publishing goes through the homelab project (homelab-publish): the project
  registers once via `pages_source` and does not push to the web server
  itself — `make deploy` is retired.

## Canonical Files

The following filenames are canonical and must not be renamed or duplicated without an explicit task:

- `README.md`
- `AGENTS.md`
- `TODO.md`
- `PHASES.md`
- `Makefile`
- `package.json`
- `.gitignore`

Tool-specific root files (for example `.aider.conf.yml`, `.aiderignore`) are permitted when a file under `tools/` declares them; see `prompts/01-contract.md` section 7.

## License

MIT. See [LICENSE](LICENSE).
