# Task: Implement the Pass Renderer feature

## TASK-DESCRIPTION
- Create: `sources/render/pass_render.h`
- Create: `sources/render/pass_render.cpp`
- Create: `tests/pass_render_test.cpp`
- Create: `tests/40-pass-render.sh`
- Modify: `sources/CMakeLists.txt`

The renderer implements the requirements in `prompts/features/04-pass-renderer.md`.
It draws the field groups as columns of label over value, uses the pass's three
colours, and embeds the pass logo as a base64 PNG data URI so the SVG stands
alone. Text that came from the pass is escaped, because a pass is untrusted
input and the output is XML.
The barcode is shown as its format and payload rather than as a symbol: this
project has no QR or PDF417 encoder yet, and a symbol that might not scan is
worse than a payload a person can read and copy. That limitation is recorded in
`documents/08-pkpass-format.md` and `TODO.md`.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `sources/render/pass_render.h`
2. `sources/render/pass_render.cpp`
3. `tests/pass_render_test.cpp`
4. `tests/40-pass-render.sh`
5. `sources/CMakeLists.txt`

## TASK-CONTEXT
<note>
`zip_reader` extracts a member by name, and `file_io` base64-encodes it. PNG
width and height are the first two big-endian 32-bit words of the IHDR chunk
data, at byte 16 of the file, so no image library is needed to scale the logo.
</note>
<constraint>
The renderer must not fail because the container moved or its images cannot be
read; it returns the card it can draw and reports the reason.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `sources/render/pass_render.h` |
| create | `sources/render/pass_render.cpp` |
| create | `tests/pass_render_test.cpp` |
| create | `tests/40-pass-render.sh` |
| modify | `sources/CMakeLists.txt` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `40-pass-render` reporting `ok`.

## TASK-FEATURES
- `prompts/features/04-pass-renderer.md`

## TASK-ACCEPTANCE
- `PASS-RENDERER-R001`
- `PASS-RENDERER-R003`
- `PASS-RENDERER-R004`
- `PASS-RENDERER-R007`
- `PASS-RENDERER-R008`
- `PASS-RENDERER-R009`
- `PASS-RENDERER-R010`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
