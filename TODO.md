# TODO

## Open Questions

* [ ] encode the other barcode formats a pass may carry: PDF417, Aztec, and
      Code 128.
      — `pkpass-render` draws QR today, which is what almost every pass uses;
      a pass whose first barcode is another format shows the payload and says the
      symbol could not be encoded.
* [ ] resolve a `.pkpasses` bundle: list the passes inside it and open one at a
      time.
      — The type is registered and opens, but `handle` treats a bundle as one
      file today.
* [ ] read the `<lang>.lproj/pass.strings` catalogs and apply the key
      indirection, so a localized pass shows its translations.
* [ ] draw the pass in the GTK4 viewer with the pass's own colours.
      — The first attempt mixed a per-pass stylesheet into the desktop theme and
      produced unreadable text for a light pass on a dark theme; the colors are
      carried by `pkpass-render` instead.
* [ ] add a Nautilus thumbnailer for `application/vnd.apple.pkpass` that runs
      `pkpass-render`.
* [ ] verify the CMS certificate chain against Apple's root when it is
      installed, and report the difference between a content check and a trusted
      chain in the report.
* [ ] add an `audit` command that reports a handler record which disagrees with
      the files on disk, as the AppImage project's `audit` does.
* [ ] write the episode plan for phases 3 and 4 in
      `prompts/episodes/02-episode-plan.md`.

## Recently Completed

* [x] make the release installer a one-line install and add a publish target
      (2026-10-23; owner instruction):
    * [x] `scripts/install.sh` now defaults to `pbannister/gnome-pkpass-integration`
      and the project asset name, so the documented `curl … | sh` needs no
      environment variables.
    * [x] the release installer registers the handler like `make install`, with
      `PKPASS_INTEGRATION_NO_HANDLER=1` to decline it.
    * [x] where no prebuilt build matches the machine, the installer builds from
      the source archive unless `PKPASS_INTEGRATION_NO_BUILD=1`.
    * [x] `make release-publish` and `scripts/release-publish.sh` tag the commit
      and create or update the GitHub release.
    * [x] `tests/08-release-publish.sh` covers the tag, push, re-upload, dirty
      tree, wrong-commit tag, and missing remote decisions with a stub `gh`.
    * [x] `tests/11-release-install.sh` covers the default repository URL, the
      handler registration, and the opt-out.

* [x] re-sync with the project skeleton (2026-10-11):
    * [x] took the skeleton's byte-identical rule files as they are: the
          contract, the workflow, the conventions, `prompts/README.md`,
          `tests/README.md`, and the five new `prompts/capabilities/`
          conventions, which split the capability rules out of the conventions
          so they are loaded only for a task that touches one.
    * [x] kept this project's sections on the append-only files
          (`documents/README.md`, `prompts/common/03-glossary.md`) by re-basing
          them onto the skeleton's new text rather than merging by hand.
    * [x] took the updated scripts (`skeleton-diff`, `context-budget`,
          `install`, `release-package`, `prompt-maturity`, `version-generate`)
          and re-applied the project's two adaptations on top: the page title in
          `site-build.sh` and the fixture number in `tests/10`.
    * [x] adopted `make context` and the always-loaded budget: 39 477 of 41 984
          bytes, inside the declared limit.
    * [x] dropped `prompts/common/00-overview.md` and `01-requirements.md`, which
          the skeleton folded into the contract and the common rules; the
          validator refuses a universal rule file the registry does not name.
    * [x] renumbered this project's reader tests to `16` and `17`, because the
          skeleton's new `tests/14-context-budget.sh` is named by the shared
          `tests/README.md` and has the stronger claim to the number.
    * [x] `scripts/skeleton-diff.sh` now reports 0 drift, 0 missing, 2 appended,
          and 2 reference adaptations; `make test` is 26 tests, all passing.
* [x] register the project pages with the homelab and publish them (2026-10-10):
    * [x] registered as `gnome-pkpass-integration` in the homelab's
          `sources/projects.yaml` with `self_published: true` and a
          `pages_source` of this project's `site.out/`; the homelab owns the
          navigation and the publish, and this project pushes nothing.
    * [x] the standard page set plus 56 normalized full-text pages are live at
          <https://labs.bannister.us/projects/gnome-pkpass-integration/>, with
          every page carrying the one homelab nav.
    * [x] the registry validates (29 projects) and the homelab's project-pages
          test reports this project OK on the standard page set and the leak
          gate.
* [x] draw the barcode, format the payload, and link the URLs (2026-10-10; owner
      instruction):
    * [x] a QR encoder written from ISO/IEC 18004: byte mode, level M, versions 1
          to 10, the Reed-Solomon blocks, the penalty-chosen mask, and the format
          and version information.
    * [x] verified two independent ways: the matrix is identical to libqrencode
          for 31 payload, version, and level combinations, and the rendered PNG
          decodes back to the exact payload with zbar.
    * [x] a JSON payload is shown re-indented with its own line structure, while
          the symbol keeps encoding the original bytes.
    * [x] `http` and `https` URLs in field values, `attributedValue` anchors, and
          the payload are clickable; no other scheme is, so a pass cannot put a
          `javascript:` link into the card.
    * [x] `pkpass-render --qr-png FILE` writes the symbol as a PNG, and the GTK4
          viewer shows it in the window.
    * [x] feature `04` gains `PASS-RENDERER-R013`–`R017`; task `07` claims them;
          tests `42-qr-encoder` and `96-qr-decode` cover the symbol.
* [x] derive the project from the skeleton and keep the shared rule corpus
      intact (2026-10-10; project start):
    * [x] copied `prompts/`, `documents/`, `records/README.md`,
          `tests/README.md`, and the reference scripts unchanged, so
          `scripts/skeleton-diff.sh` reports no rule drift.
    * [x] kept the skeleton's 00–13 tests; `make test` runs 22 tests and all pass.
    * [x] declared the project settings in `README.md`: automatic task commits
          enabled, the privacy boundary, and that there are no live-state tests.
* [x] read a pass, and check it (2026-10-10; phases 2):
    * [x] SHA-1, a JSON value and parser, and a ZIP reader that honours each
          member's own compression method and checks the CRC.
    * [x] `pass.json`: the top-level keys, all five styles, the five field
          groups, the field keys, the colours (`rgb(86, 39, 126)` with its
          spaces, and the hex forms), both barcode shapes, and the locations.
    * [x] `manifest.json` digests checked one by one, with missing, mismatched,
          and unlisted members reported separately.
    * [x] the detached PKCS#7 signature checked through `openssl`, reported as
          absent, verified, invalid, or unavailable.
    * [x] feature `03-pkpass-reader.md`, task `02`, and tests
          `16-sha1`, `17-json-reader`, `30-zip-reader`, `31-pkpass-reader`.
* [x] draw a pass (2026-10-10; phase 3):
    * [x] one standalone SVG card: the pass's colours, the field groups as
          label-over-value columns, the logo embedded as a base64 PNG chosen
          from the 2x/1x/3x copies, and the barcode's format, altText, and
          payload.
    * [x] every pass-supplied string XML-escaped, because a pass is untrusted
          input and the card is XML.
    * [x] feature `04-pass-renderer.md`, task `03`, and tests
          `pass-render-test` and `40-pass-render`.
* [x] integrate the pass types with the desktop (2026-10-10; phase 3):
    * [x] the definition of `application/vnd.apple.pkpass` and
          `application/vnd.apple.pkpasses`, with their globs, magic, and this
          project's icon.
    * [x] the handler entry, the three icons, the record, and
          `xdg-mime default`; a definition the project did not write is backed
          up and restored byte for byte.
    * [x] feature `05-pkpass-handler.md`, task `04`, and tests
          `handler-integrator-test` and `90-handler-mime-definition`.
* [x] ship the command-line tools and the build (2026-10-10; phase 3):
    * [x] `pkpass-inspect` (report, `--json`, `--signature`),
          `pkpass-render`, and `pkpass-integrate` (`handler`, `handle`).
    * [x] the exit-code contract: 0 for a report including a mismatched
          manifest, 1 for a file that cannot be read, 2 for a usage error.
    * [x] `scripts/program-build.sh`, `scripts/program-install.sh`, and
          `scripts/release-stage.sh`; the Makefile drives build, install, and
          release.
    * [x] the GTK4 viewer, built only where the GTK4 development files are
          present, with `handle` falling back to printed commands.
    * [x] feature `06-command-line-tools.md`, task `05`, and test
          `60-command-line-tools`.
* [x] make `make install` register the handler (2026-10-10; owner instruction):
    * [x] `scripts/program-install.sh` now ends by running the freshly installed
          `pkpass-integrate handler install`, so the desktop entry names the
          installed program rather than one in the build tree.
    * [x] `PKPASS_INTEGRATION_NO_HANDLER=1` installs the programs and registers
          nothing, and `pkpass-integrate handler uninstall` reverses a
          registration.
    * [x] a registration that cannot happen leaves the programs installed, prints
          why, and exits nonzero, so an unattended install cannot report success
          with the association missing.
    * [x] `tests/91-install-handler.sh` covers all three cases against a
          throwaway `HOME` and prefix; feature `05` gains `PKPASS-HANDLER-R021`
          and task `06` claims it.
* [x] verify the GTK4 viewer instead of only reviewing it (2026-10-10):
    * [x] the GTK4 development files were installed on the build host, so the
          viewer target builds; its source compiles clean under the project's
          `-Wall -Wextra -Wpedantic -Werror`.
    * [x] `tests/95-viewer.sh` starts a private Xvfb display and checks the
          window's title and WM class through the X server, so a viewer that
          compiles and then exits can no longer pass unnoticed.
    * [x] the window carries the `pkpass-viewer` WM class the handler entry
          names, and `pkpass-integrate handle` puts the same window up.
* [x] research the format before writing the reader (2026-10-10; phase 2):
    * [x] `documents/08-pkpass-format.md` and
          `documents/09-mime-type-association.md` record the format, the MIME
          rules, and the sources.
    * [x] the synthetic fixture at `dataflow.in/pkpass/sample/` mirrors the
          structure of a real pass without carrying anyone's data; the real
          example that prompted the project is not committed.
