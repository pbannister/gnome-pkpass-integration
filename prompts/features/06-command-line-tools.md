# Feature: Command-Line Tools

## Purpose

Everything the desktop does must also be available on the command line, so a
pass can be read, checked, rendered, and registered from a script.

## Requirements

- `COMMAND-LINE-TOOLS-R001` — `pkpass-inspect FILE` must print a human-readable report of the container, the pass, the fields, the barcodes, the locations, the images, the manifest result, the signature presence, and any warning.
- `COMMAND-LINE-TOOLS-R002` — `pkpass-inspect --json FILE` must print the same facts as one JSON document.
- `COMMAND-LINE-TOOLS-R003` — `pkpass-inspect --signature FILE` must also check the PKCS#7 signature and report its status and signer.
- `COMMAND-LINE-TOOLS-R004` — `pkpass-inspect` must report a mismatched manifest in its output but must still exit zero, so a caller can read the report; the JSON member `verification.manifestOk` is the machine-readable signal.
- `COMMAND-LINE-TOOLS-R005` — `pkpass-render FILE` must write an SVG card to standard output, and `--output FILE` must write it to a file.
- `COMMAND-LINE-TOOLS-R006` — `pkpass-render --back` must draw the reverse, and `--no-images` must omit the embedded images.
- `COMMAND-LINE-TOOLS-R007` — `pkpass-integrate handler status|install|uninstall` must drive the handler integration and print the result of each action.
- `COMMAND-LINE-TOOLS-R008` — `pkpass-integrate handle FILE` must be the desktop entry point.
- `COMMAND-LINE-TOOLS-R009` — Every tool must print `<name> <version> (build <counter>)` for `--version`, where the version is the build-time version.
- `COMMAND-LINE-TOOLS-R010` — Every tool must print usage for `--help` and exit zero.
- `COMMAND-LINE-TOOLS-R011` — An unknown option, an unknown command, an unknown handler subcommand, or a missing required argument must print usage and exit with status 2.
- `COMMAND-LINE-TOOLS-R012` — A file that cannot be read must print `<program>: <message>` on standard error and exit with status 1.
- `COMMAND-LINE-TOOLS-R013` — `pkpass-inspect` and `pkpass-render` must accept exactly one file.
- `COMMAND-LINE-TOOLS-R014` — `pkpass-render` must print a warning on standard error, and still write the card, when the images could not be read.
- `COMMAND-LINE-TOOLS-R015` — `pkpass-viewer FILE.pkpass` must open a window titled with the pass description and carrying the `pkpass-viewer` WM class, and must open an explanatory window rather than exiting silently when the file is not a readable pass.

## Behavior

- `pkpass-inspect FILE` on the fixture prints `manifest: 3 entries, every SHA-1 digest matches`.
- `pkpass-inspect --json FILE` parses as JSON and reports `verification.manifestOk` true.
- On a tampered pass, the report names the mismatched member and the JSON says `manifestOk` false.
- `pkpass-render --output card.svg FILE` leaves a non-empty SVG carrying the embedded logo.
- `pkpass-inspect` with no file argument exits 2; with a path that does not exist it exits 1.
- `pkpass-viewer` on the fixture leaves a window named for the pass on the display, and `pkpass-integrate handle` puts the same window up.

## Dependencies

- `03-pkpass-reader.md`
- `04-pass-renderer.md`
- `05-pkpass-handler.md`
