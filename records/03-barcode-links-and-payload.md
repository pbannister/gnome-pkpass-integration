# Record: the barcode, the payload, and the links

## Outcome

`pkpass-render` now draws the barcode as a real QR symbol, shows a JSON payload
re-indented, and makes `http` and `https` URLs clickable. The GTK4 viewer shows
the same symbol and the same links.

The work is in commit `1045efb`.

- `sources/qr/qr_encoder.h` and `.cpp` — a QR encoder written from
  ISO/IEC 18004: 8-bit byte mode, error correction level M, versions 1 to 10,
  the Reed-Solomon block structure of each version and level, the mask chosen by
  the four penalty rules, and the format and version information.
- `sources/render/` — the card draws the symbol, keeps a JSON payload's own line
  structure and indentation, and splits text into literal and link runs.
- `sources/tools/pkpass_render.cpp` — `--qr-png FILE` writes the symbol as a PNG.
- `sources/tools/pkpass_viewer_ui.cpp` — the window shows the symbol, re-indents
  the payload, and turns a URL into a clickable link.
- `tests/qr_encoder_test.cpp`, `tests/42-qr-encoder.sh` — the encoder against
  ground truth from libqrencode.
- `tests/96-qr-decode.sh` — the rendered PNG read back by a decoder.
- `tests/pass_render_test.cpp`, `tests/60-command-line-tools.sh` — the card's
  symbol, links, formatting, and the new option.

`make test` is now 25 tests, all passing.

## Decisions

- **The symbol is verified against an independent implementation, not by eye.**
  A QR code that does not scan is worse than no QR code, and a matrix can look
  entirely plausible while being wrong. The expected matrices in the test come
  from libqrencode in explicit byte mode, and the test forces the mask
  libqrencode chose, because two correct encoders may otherwise pick different
  but equally valid masks. Separately, `tests/96-qr-decode.sh` renders the
  symbol and has a decoder read it back.
- **`PASS-RENDERER-R009` changed meaning.** It previously required that no symbol
  be drawn, because the project had no encoder. The owner asked for the symbol,
  so the identifier now names the opposite requirement. The identifier is
  stable; the requirement it names is not, and that is worth knowing when
  reading an older acceptance list.
- **Only QR is encoded.** A pass may carry PDF417, Aztec, or Code 128. Supporting
  one format well, verified, is worth more than four formats unverified; a pass
  whose first barcode is another format shows its payload and a note that the
  symbol could not be encoded. Carrying the others is an open `TODO.md` item.
- **Level M and byte mode.** Byte mode is what a barcode message is, whatever
  the issuer put in it, and it does not depend on the payload happening to look
  alphanumeric. Level M is the usual choice for a pass and holds 213 bytes at
  version 10; a longer payload is refused rather than drawn.
- **A pass cannot put an arbitrary scheme into the card.** Only `http` and
  `https` are linkified, so a `javascript:` or `data:` URL stays text. Only
  `attributedValue` is treated as markup; a plain `value` is literal, so a `<` in
  it is drawn as a `<` rather than dropped as a tag.
- **Formatting the payload does not change what is encoded.** The symbol always
  encodes the message bytes exactly as the issuer wrote them; the re-indenting
  applies to the text drawn below it.
- **The verification tooling lives outside the project.** The decoder and a
  second encoder were installed into a gitignored `.tmp-verify` directory with
  `pip install --target`, which needs no root and does not touch the project.
  The project's own decode test therefore skips on a host with no decoder, and
  that skip is the honest statement that no scan check was made there.

## Verification

- The matrix is identical to libqrencode for 31 combinations of payload length,
  version (1 to 10), and error correction level (L, M, Q, H), comparing module
  for module at the mask libqrencode chose.
- Four rendered PNGs (levels L, M, Q, H) decoded with zbar back to the exact
  payload bytes.
- `tests/42-qr-encoder.sh` passes, and `tests/96-qr-decode.sh` passes with a
  decoder on `PYTHONPATH` and skips cleanly without one, on 2026-10-10.
- `make test` — 25 tests, all pass, with the decoder available, on 2026-10-10.
- `make site` — the page set builds and `scripts/leak-gate.sh site.out` reports
  `clean`, on 2026-10-10.
- `scripts/skeleton-diff.sh` from the skeleton — 0 drift.
- Two real defects were found and fixed during the work, both by the
  independent comparison: the format-information area was overwriting the timing
  modules at (8,6) and (6,8), and the alignment-pattern corner test skipped all
  four patterns instead of the three that overlap finder patterns, which left
  version 2 with no alignment pattern at all.

## Commits

- `1045efb` — feat: draw the barcode, format the payload, and link the URLs. The
  encoder, the renderer, the viewer, the option, the tests, the feature
  requirements, the new task, and the documentation.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the work commit above rather than itself.
