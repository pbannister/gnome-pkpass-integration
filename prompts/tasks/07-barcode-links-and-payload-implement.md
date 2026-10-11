# Task: Draw the barcode, format the payload, and link the URLs

## TASK-DESCRIPTION
- Create: `sources/qr/qr_encoder.h`
- Create: `sources/qr/qr_encoder.cpp`
- Create: `tests/qr_encoder_test.cpp`
- Create: `tests/42-qr-encoder.sh`
- Create: `tests/96-qr-decode.sh`
- Modify: `sources/render/pass_render.h`
- Modify: `sources/render/pass_render.cpp`
- Modify: `sources/tools/pkpass_render.cpp`
- Modify: `sources/tools/pkpass_viewer_ui.cpp`
- Modify: `sources/CMakeLists.txt`
- Modify: `tests/pass_render_test.cpp`
- Modify: `tests/60-command-line-tools.sh`
- Modify: `prompts/features/04-pass-renderer.md`
- Modify: `prompts/tasks/00-tasks.md`
- Modify: `documents/08-pkpass-format.md`
- Modify: `README.md`
- Modify: `TODO.md`

The work implements the requirements in `prompts/features/04-pass-renderer.md`,
`PASS-RENDERER-R009` and `R013` through `R017`.

The symbol is the substantial part. The encoder is written from ISO/IEC 18004:
8-bit byte mode, error correction level M, versions 1 to 10, the Reed-Solomon
blocks of each version and level, the mask chosen by the four penalty rules, and
the format and version information.
A symbol that might not scan is worse than no symbol, so it is verified against
an independent encoder module for module and, where a decoder is installed, by
reading the rendered PNG back.

Mask selection is left to the encoder; `encode_masked` exists so a test can
force the mask an external encoder chose, because two correct encoders may
otherwise pick different but equally valid masks.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `sources/qr/qr_encoder.h`
2. `sources/qr/qr_encoder.cpp`
3. `tests/qr_encoder_test.cpp`
4. `tests/42-qr-encoder.sh`
5. `tests/96-qr-decode.sh`
6. `sources/render/pass_render.h`
7. `sources/render/pass_render.cpp`
8. `sources/tools/pkpass_render.cpp`
9. `sources/tools/pkpass_viewer_ui.cpp`
10. `sources/CMakeLists.txt`
11. `tests/pass_render_test.cpp`
12. `tests/60-command-line-tools.sh`
13. `prompts/features/04-pass-renderer.md`
14. `prompts/tasks/00-tasks.md`
15. `documents/08-pkpass-format.md`
16. `README.md`
17. `TODO.md`

## TASK-CONTEXT
<note>
`PASS-RENDERER-R009` used to require the opposite: that no symbol be drawn,
because the project had no encoder. The owner asked for the symbol, so the
requirement now names the opposite behaviour under the same identifier; the
change is recorded in `records/03-barcode-links-and-payload.md`.
Only `http` and `https` become links, so a hostile pass cannot put a
`javascript:` URL into the card. `attributedValue` is the only field that may
carry HTML.
</note>
<constraint>
The ground-truth matrices in `tests/qr_encoder_test.cpp` come from libqrencode.
They must not be regenerated from this project's own encoder, or the test would
only prove the encoder agrees with itself.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `sources/qr/qr_encoder.h` |
| create | `sources/qr/qr_encoder.cpp` |
| create | `tests/qr_encoder_test.cpp` |
| create | `tests/42-qr-encoder.sh` |
| create | `tests/96-qr-decode.sh` |
| modify | `sources/render/pass_render.h` |
| modify | `sources/render/pass_render.cpp` |
| modify | `sources/tools/pkpass_render.cpp` |
| modify | `sources/tools/pkpass_viewer_ui.cpp` |
| modify | `sources/CMakeLists.txt` |
| modify | `tests/pass_render_test.cpp` |
| modify | `tests/60-command-line-tools.sh` |
| modify | `prompts/features/04-pass-renderer.md` |
| modify | `prompts/tasks/00-tasks.md` |
| modify | `documents/08-pkpass-format.md` |
| modify | `README.md` |
| modify | `TODO.md` |

## TASK-VERIFY
- Run: `make test` from the repository root, with a decoder available
  (`PYTHONPATH` set to a directory holding the `pyzbar` module, or `zbarimg`
  installed).
- Expected: exit status 0, with `42-qr-encoder` and `96-qr-decode` both reporting `ok`, and `96-qr-decode` reporting a skip rather than a failure where no decoder is installed.

## TASK-FEATURES
- `prompts/features/04-pass-renderer.md`

## TASK-ACCEPTANCE
- `PASS-RENDERER-R009`
- `PASS-RENDERER-R013`
- `PASS-RENDERER-R014`
- `PASS-RENDERER-R015`
- `PASS-RENDERER-R016`
- `PASS-RENDERER-R017`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
