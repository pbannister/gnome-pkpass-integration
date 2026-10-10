# Feature: Pass Renderer

## Purpose

The project must draw a pass so a person can see it without Wallet: one SVG
document that stands alone, carrying the pass's own colours, logo, fields, and
barcode payload.

## Requirements

- `PASS-RENDERER-R001` — The renderer must produce one self-contained SVG document for a pass.
- `PASS-RENDERER-R002` — The renderer must draw the `headerFields`, `primaryFields`, `secondaryFields`, and `auxiliaryFields` groups as columns of label over value, in that order.
- `PASS-RENDERER-R003` — The renderer must use the pass's `backgroundColor`, `foregroundColor`, and `labelColor`, and must fall back to a defined default when a colour is absent or unreadable.
- `PASS-RENDERER-R004` — The renderer must embed the pass logo as a base64 PNG data URI, choosing among `logo@2x.png`, `logo.png`, and `logo@3x.png` in that order.
- `PASS-RENDERER-R005` — The renderer must scale the logo to fit a fixed box without distorting its aspect ratio, reading the dimensions from the PNG header.
- `PASS-RENDERER-R006` — The renderer must show the first barcode's format label, its `altText`, and its `message`.
- `PASS-RENDERER-R007` — The renderer must draw the `backFields` on request, and must not draw them otherwise.
- `PASS-RENDERER-R008` — The renderer must escape the XML metacharacters in every string that came from the pass.
- `PASS-RENDERER-R009` — The renderer must not draw a scannable-looking symbol it cannot encode; until an encoder exists it shows the barcode's format and payload instead.
- `PASS-RENDERER-R010` — The renderer must still produce a card when the container cannot be reopened for images, and must report why the images are missing.
- `PASS-RENDERER-R011` — The renderer must wrap long field text instead of running it off the card.
- `PASS-RENDERER-R012` — No public interface may throw.

## Behavior

- Given a pass with a purple background, the SVG carries `rgb(86,39,126)`.
- Given a logo larger than the logo box, the drawn size fits the box and keeps the logo's shape.
- Given a field value containing `<script>`, the SVG contains no `<script>` element.
- Given a request for the reverse, the card names the back field labels and the front does not.
- Given a path whose file has been removed, the card is still returned.

## Dependencies

- `03-pkpass-reader.md`
