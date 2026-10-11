#!/bin/sh
#
# QR decode test: the rendered symbol read back by an independent decoder.
#
# Tier: tool-gated (cmake, python3, and a decoder).  A matrix that matches
# libqrencode is strong evidence, but only a decoder shows that the symbol a
# phone would see carries the payload.  This renders the pass's barcode to a PNG
# with `pkpass-render --qr-png` and decodes it with whichever decoder is
# installed:
#
#   * `zbarimg`, from zbar-tools;
#   * `python3` with the `pyzbar` module (`pip install --target DIR pyzbar`,
#     with `PYTHONPATH=DIR` when running the tests).
#
# Neither is installed by default, so the test skips cleanly where there is no
# decoder; that skip is the honest statement that no scan check was made.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
skip_unless_tool python3
build_program

# Choose a decoder before doing any work, so an absent one is a clean skip.
DECODER=none
if command -v zbarimg >/dev/null 2>&1; then
    DECODER=zbarimg
elif python3 -c 'import pyzbar' >/dev/null 2>&1; then
    DECODER=pyzbar
else
    skip_test 'missing tool: a QR decoder (zbarimg, or python3 with the pyzbar module)'
fi

DIRECTORY_TEMPORARY=$(mktemp -d)
trap 'rm -rf "$DIRECTORY_TEMPORARY"' EXIT HUP INT TERM

FILE_PKPASS="$DIRECTORY_TEMPORARY/sample.pkpass"
FILE_PNG="$DIRECTORY_TEMPORARY/barcode.png"
build_synthetic_pkpass "$DIRECTORY_FIXTURES" "$FILE_PKPASS"

# The payload the fixture's pass.json names, read rather than hardcoded, so the
# test cannot drift from the fixture.
TEXT_EXPECTED=$(python3 - "$DIRECTORY_FIXTURES/pass.json" <<'PYTHON_MESSAGE'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    pass_json = json.load(handle)
print(pass_json["barcode"]["message"])
PYTHON_MESSAGE
)

if ! "$DIRECTORY_BUILD/pkpass-render" --qr-png "$FILE_PNG" "$FILE_PKPASS" >/dev/null; then
    fail_test "$NAME_TEST" 'pkpass-render --qr-png failed'
fi
[ -s "$FILE_PNG" ] || fail_test "$NAME_TEST" '--qr-png wrote no file'

case "$DECODER" in
    zbarimg)
        TEXT_DECODED=$(zbarimg --quiet --raw "$FILE_PNG" 2>/dev/null || true)
        ;;
    pyzbar)
        TEXT_DECODED=$(python3 - "$FILE_PNG" <<'PYTHON_DECODE'
import sys

from pyzbar.pyzbar import decode
from PIL import Image

results = decode(Image.open(sys.argv[1]))
print(results[0].data.decode("utf-8") if results else "")
PYTHON_DECODE
        )
        ;;
esac

if [ "$TEXT_DECODED" != "$TEXT_EXPECTED" ]; then
    fail_test "$NAME_TEST" "the decoder read '$TEXT_DECODED', not the barcode message"
fi

pass_test "$NAME_TEST"
