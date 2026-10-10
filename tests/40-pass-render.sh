#!/bin/sh
#
# Pass renderer test (sources/render/pass_render.cpp).
#
# Tier: tool-gated (cmake, a C++ compiler, python3 to build the fixture).  The
# card is checked through the library and through the `pkpass-render` tool, so
# both the SVG text and the file the tool writes are covered.
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

if ! "$DIRECTORY_BUILD/pass-render-test" "$FILE_PKPASS"; then
    fail_test "$NAME_TEST" 'pass-render-test reported a failure'
fi

# The tool writes the same card to a file, and refuses a bad invocation.
FILE_SVG="$DIRECTORY_TEMPORARY/card.svg"
"$DIRECTORY_BUILD/pkpass-render" --output "$FILE_SVG" "$FILE_PKPASS"
if [ ! -s "$FILE_SVG" ]; then
    fail_test "$NAME_TEST" 'pkpass-render wrote no SVG'
fi
grep -q '<svg' "$FILE_SVG" || fail_test "$NAME_TEST" 'the written SVG has no root element'
grep -q 'data:image/png;base64,' "$FILE_SVG" \
    || fail_test "$NAME_TEST" 'the written SVG did not embed the pass logo'

if "$DIRECTORY_BUILD/pkpass-render" >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'pkpass-render accepted a missing file argument'
fi

pass_test "$NAME_TEST"
