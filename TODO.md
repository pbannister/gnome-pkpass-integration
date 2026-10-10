# TODO

## Open Questions

* [ ] encode a barcode symbol so `pkpass-render` draws a scannable code rather
      than the payload text.
      — A QR encoder is the largest missing piece of a pass viewer, and the
      limitation is documented in `documents/08-pkpass-format.md`. The symbol
      must be verified by decoding it, not by eye, so the work includes a decoder
      in the test.
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
* [ ] register the project pages with the homelab (`pages_source`) and publish.

## Recently Completed

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
          `14-sha1`, `15-json-reader`, `30-zip-reader`, `31-pkpass-reader`.
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
* [x] research the format before writing the reader (2026-10-10; phase 2):
    * [x] `documents/08-pkpass-format.md` and
          `documents/09-mime-type-association.md` record the format, the MIME
          rules, and the sources.
    * [x] the synthetic fixture at `dataflow.in/pkpass/sample/` mirrors the
          structure of a real pass without carrying anyone's data; the real
          example that prompted the project is not committed.
