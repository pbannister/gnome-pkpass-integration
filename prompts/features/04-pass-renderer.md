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
- `PASS-RENDERER-R009` — The renderer must draw the first barcode as a real QR symbol, encoded from the message bytes at error correction level M, in the smallest version that fits; when the payload does not fit a supported version it must say so instead of drawing a symbol that would not scan.
- `PASS-RENDERER-R010` — The renderer must still produce a card when the container cannot be reopened for images, and must report why the images are missing.
- `PASS-RENDERER-R011` — The renderer must wrap long field text instead of running it off the card.
- `PASS-RENDERER-R012` — No public interface may throw.
- `PASS-RENDERER-R013` — The renderer must show the barcode payload re-indented when it is a JSON object or array, keeping every line's own indentation, and must not change the bytes the symbol encodes.
- `PASS-RENDERER-R014` — The renderer must make an `http` or `https` URL in a field value, an `attributedValue` anchor, or the payload clickable, and must not make any other scheme clickable.
- `PASS-RENDERER-R015` — The renderer must keep a literal newline in a field value as a line break.
- `PASS-RENDERER-R016` — The renderer must treat only `attributedValue` as markup; a plain `value` is literal text and any angle bracket in it is drawn as text.
- `PASS-RENDERER-R017` — `pkpass-render --qr-png FILE` must write the barcode as a PNG, so the symbol can be scanned from the card or copied to another screen.

## Behavior

- Given a pass with a purple background, the SVG carries `rgb(86,39,126)`.
- Given a logo larger than the logo box, the drawn size fits the box and keeps the logo's shape.
- Given a field value containing `<script>`, the SVG contains no `<script>` element.
- Given a request for the reverse, the card names the back field labels and the front does not.
- Given a path whose file has been removed, the card is still returned.
- Given a barcode whose message is `{"ticket":"0000000001","seat":"A1"}`, the card carries a scannable QR symbol of exactly those bytes and shows the payload on three indented lines.
- Given a field value holding `https://example.invalid/profile/1`, the SVG carries an anchor to that URL and no anchor to a `javascript:` URL.
- Given a value with a newline in it, the card draws it as two lines.

## Dependencies

- `03-pkpass-reader.md`
