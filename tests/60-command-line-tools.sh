#!/bin/sh
#
# Command-line tools test (prompts/features/06-command-line-tools.md).
#
# Tier: tool-gated (cmake, a C++ compiler, python3).  Exercises the three public
# programs the way a person or a script uses them: the report, the JSON form,
# the renderer, and the exit codes, including a tampered pass whose manifest
# digest no longer matches.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
skip_unless_tool python3
build_program

DIRECTORY_TEMPORARY=$(mktemp -d)
trap 'rm -rf "$DIRECTORY_TEMPORARY"' EXIT HUP INT TERM

FILE_PKPASS="$DIRECTORY_TEMPORARY/sample.pkpass"
build_synthetic_pkpass "$DIRECTORY_FIXTURES" "$FILE_PKPASS"

PROGRAM_INSPECT="$DIRECTORY_BUILD/pkpass-inspect"
PROGRAM_RENDER="$DIRECTORY_BUILD/pkpass-render"
PROGRAM_INTEGRATE="$DIRECTORY_BUILD/pkpass-integrate"

# --- version and help --------------------------------------------------------
for program in "$PROGRAM_INSPECT" "$PROGRAM_RENDER" "$PROGRAM_INTEGRATE"; do
    if ! "$program" --version | grep -q '^pkpass-'; then
        fail_test "$NAME_TEST" "$program --version did not name the program"
    fi
    "$program" --help >/dev/null || fail_test "$NAME_TEST" "$program --help failed"
done

# --- a whole pass is reported ------------------------------------------------
FILE_REPORT="$DIRECTORY_TEMPORARY/report.txt"
"$PROGRAM_INSPECT" "$FILE_PKPASS" > "$FILE_REPORT"

for expected in \
    'style: generic' \
    'passTypeIdentifier: pass.com.example.walletpass' \
    'serialNumber: 0000000001' \
    'barcode: QR encoding=utf-8' \
    'manifest: 3 entries, every SHA-1 digest matches' \
    'signature: absent' \
    'image: icon.png'; do
    grep -qF "$expected" "$FILE_REPORT" \
        || fail_test "$NAME_TEST" "the report is missing: $expected"
done

# --- the JSON form agrees with the report ------------------------------------
FILE_JSON="$DIRECTORY_TEMPORARY/report.json"
"$PROGRAM_INSPECT" --json "$FILE_PKPASS" > "$FILE_JSON"

python3 - "$FILE_JSON" <<'PYTHON_CHECK' || fail_test "$NAME_TEST" 'the JSON report is wrong'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    report = json.load(handle)

assert report["style"] == "generic", report["style"]
assert report["serialNumber"] == "0000000001"
assert report["verification"]["manifestOk"] is True
assert report["verification"]["manifestMismatched"] == 0
assert report["verification"]["hasSignature"] is False
assert report["barcodes"][0]["format"] == "PKBarcodeFormatQR"
assert report["barcodes"][0]["altText"] == "ID: 0000000001"
assert report["fields"]["primaryFields"][0]["value"].startswith("Ada")
assert report["backgroundColor"] == {"red": 12, "green": 34, "blue": 56, "alpha": 255}
assert len(report["locations"]) == 1
PYTHON_CHECK

# --- a tampered pass is reported as such, not accepted silently --------------
DIRECTORY_TAMPERED="$DIRECTORY_TEMPORARY/tampered"
mkdir -p "$DIRECTORY_TAMPERED"
cp -R "$DIRECTORY_FIXTURES"/. "$DIRECTORY_TAMPERED"/
printf '\n' >> "$DIRECTORY_TAMPERED/pass.json"
FILE_TAMPERED="$DIRECTORY_TEMPORARY/tampered.pkpass"
build_synthetic_pkpass "$DIRECTORY_TAMPERED" "$FILE_TAMPERED"

"$PROGRAM_INSPECT" --json "$FILE_TAMPERED" > "$DIRECTORY_TEMPORARY/tampered.json"
python3 - "$DIRECTORY_TEMPORARY/tampered.json" <<'PYTHON_CHECK' || fail_test "$NAME_TEST" 'a tampered pass was not reported'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    report = json.load(handle)

assert report["verification"]["manifestOk"] is False, "the tampered manifest was accepted"
assert report["verification"]["manifestMismatched"] == 1, report["verification"]
PYTHON_CHECK

# --- usage errors are exit code 2 --------------------------------------------
if "$PROGRAM_INSPECT" >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-inspect accepted no file argument'
fi
if "$PROGRAM_INSPECT" --nonsense "$FILE_PKPASS" >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-inspect accepted an unknown option'
fi
if "$PROGRAM_INSPECT" a.pkpass b.pkpass >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-inspect accepted two files'
fi
if "$PROGRAM_INTEGRATE" nonsense >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-integrate accepted an unknown command'
fi
if "$PROGRAM_INTEGRATE" handler nonsense >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-integrate accepted an unknown handler subcommand'
fi
if "$PROGRAM_INTEGRATE" handle >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-integrate handle accepted no file'
fi

# --- the handler entry point works without a display -------------------------
# The desktop runs `pkpass-integrate handle FILE`; with no display and no viewer
# it must still say what it would do, and succeed.
FILE_HANDLE="$DIRECTORY_TEMPORARY/handle.txt"
env -u DISPLAY -u WAYLAND_DISPLAY "$PROGRAM_INTEGRATE" handle "$FILE_PKPASS" > "$FILE_HANDLE"
grep -q 'pkpass-inspect' "$FILE_HANDLE" \
    || fail_test "$NAME_TEST" 'the handler entry point printed no equivalent command'

# --- a missing path is refused ----------------------------------------------
if "$PROGRAM_INSPECT" "$DIRECTORY_TEMPORARY/absent.pkpass" >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-inspect accepted a path that does not exist'
fi

pass_test "$NAME_TEST"
