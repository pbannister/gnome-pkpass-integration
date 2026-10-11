# Task: Implement the PKPASS Reader feature

## TASK-DESCRIPTION
- Create: `sources/crypto/sha1.h`
- Create: `sources/crypto/sha1.cpp`
- Create: `sources/json/json_reader.h`
- Create: `sources/json/json_reader.cpp`
- Create: `sources/util/file_io.h`
- Create: `sources/util/file_io.cpp`
- Create: `sources/zip/zip_reader.h`
- Create: `sources/zip/zip_reader.cpp`
- Create: `sources/pkpass/pass_model.h`
- Create: `sources/pkpass/pkpass_reader.h`
- Create: `sources/pkpass/pkpass_reader.cpp`
- Create: `sources/pkpass/pkpass_verify.h`
- Create: `sources/pkpass/pkpass_verify.cpp`
- Create: `sources/CMakeLists.txt`
- Create: `tests/test_harness.h`
- Create: `tests/sha1_test.cpp`
- Create: `tests/json_reader_test.cpp`
- Create: `tests/zip_reader_test.cpp`
- Create: `tests/pkpass_reader_test.cpp`
- Create: `tests/16-sha1.sh`
- Create: `tests/17-json-reader.sh`
- Create: `tests/30-zip-reader.sh`
- Create: `tests/31-pkpass-reader.sh`
- Modify: `tests/lib/test_helpers.sh`
- Create: `dataflow.in/pkpass/sample/pass.json`
- Create: `dataflow.in/pkpass/sample/manifest.json`
- Create: `dataflow.in/pkpass/sample/icon.png`
- Create: `dataflow.in/pkpass/sample/logo.png`

The reader implements the requirements in `prompts/features/03-pkpass-reader.md`.
The ZIP reader must honour each member's own compression method and check the
CRC, because Apple's own passes mix stored and deflated members. The colour
parser must accept `rgb(86, 39, 126)` with its spaces, and the barcode reader
must accept the legacy singular `barcode` object as well as the `barcodes`
array. The fixture under `dataflow.in/pkpass/sample/` is a synthetic pass that
mirrors the structure of a real one without carrying anyone's data; the shell
tests zip it into a `.pkpass`.
`manifest.json` records the SHA-1 of every member except itself and `signature`,
so it is regenerated whenever a sample member changes.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `sources/crypto/sha1.h`
2. `sources/crypto/sha1.cpp`
3. `sources/json/json_reader.h`
4. `sources/json/json_reader.cpp`
5. `sources/util/file_io.h`
6. `sources/util/file_io.cpp`
7. `sources/zip/zip_reader.h`
8. `sources/zip/zip_reader.cpp`
9. `sources/pkpass/pass_model.h`
10. `sources/pkpass/pkpass_reader.h`
11. `sources/pkpass/pkpass_reader.cpp`
12. `sources/pkpass/pkpass_verify.h`
13. `sources/pkpass/pkpass_verify.cpp`
14. `sources/CMakeLists.txt`
15. `tests/test_harness.h`
16. `tests/sha1_test.cpp`
17. `tests/json_reader_test.cpp`
18. `tests/zip_reader_test.cpp`
19. `tests/pkpass_reader_test.cpp`
20. `tests/16-sha1.sh`
21. `tests/17-json-reader.sh`
22. `tests/30-zip-reader.sh`
23. `tests/31-pkpass-reader.sh`
24. `tests/lib/test_helpers.sh`
25. `dataflow.in/pkpass/sample/pass.json`
26. `dataflow.in/pkpass/sample/manifest.json`
27. `dataflow.in/pkpass/sample/icon.png`
28. `dataflow.in/pkpass/sample/logo.png`

## TASK-CONTEXT
<note>
`documents/08-pkpass-format.md` records the format: a `.pkpass` is a ZIP, a
manifest maps each member to a lowercase SHA-1 hex digest, and the signature is
a detached PKCS#7 structure over the manifest bytes. The CMS digest is not the
manifest's SHA-1, so the signature check shells out to `openssl` and reports
`unavailable` where that tool is absent. The build treats warnings as errors.
</note>
<constraint>
The fixture is synthetic. Do not commit a real pass: a pass carries its
holder's name, identifiers, and barcode payload, and this repository is
published.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `sources/crypto/sha1.h` |
| create | `sources/crypto/sha1.cpp` |
| create | `sources/json/json_reader.h` |
| create | `sources/json/json_reader.cpp` |
| create | `sources/util/file_io.h` |
| create | `sources/util/file_io.cpp` |
| create | `sources/zip/zip_reader.h` |
| create | `sources/zip/zip_reader.cpp` |
| create | `sources/pkpass/pass_model.h` |
| create | `sources/pkpass/pkpass_reader.h` |
| create | `sources/pkpass/pkpass_reader.cpp` |
| create | `sources/pkpass/pkpass_verify.h` |
| create | `sources/pkpass/pkpass_verify.cpp` |
| create | `sources/CMakeLists.txt` |
| create | `tests/test_harness.h` |
| create | `tests/sha1_test.cpp` |
| create | `tests/json_reader_test.cpp` |
| create | `tests/zip_reader_test.cpp` |
| create | `tests/pkpass_reader_test.cpp` |
| create | `tests/16-sha1.sh` |
| create | `tests/17-json-reader.sh` |
| create | `tests/30-zip-reader.sh` |
| create | `tests/31-pkpass-reader.sh` |
| modify | `tests/lib/test_helpers.sh` |
| create | `dataflow.in/pkpass/sample/pass.json` |
| create | `dataflow.in/pkpass/sample/manifest.json` |
| create | `dataflow.in/pkpass/sample/icon.png` |
| create | `dataflow.in/pkpass/sample/logo.png` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `14-sha1`, `15-json-reader`, `30-zip-reader`, and `31-pkpass-reader` each reporting `ok`.

## TASK-FEATURES
- `prompts/features/03-pkpass-reader.md`

## TASK-ACCEPTANCE
- `PKPASS-READER-R001`
- `PKPASS-READER-R002`
- `PKPASS-READER-R003`
- `PKPASS-READER-R010`
- `PKPASS-READER-R011`
- `PKPASS-READER-R016`
- `PKPASS-READER-R017`
- `PKPASS-READER-R019`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
