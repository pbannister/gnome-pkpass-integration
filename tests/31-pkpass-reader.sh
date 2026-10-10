#!/bin/sh
#
# PKPASS reader test (sources/pkpass/pkpass_reader.cpp).
#
# Tier: tool-gated (cmake, a C++ compiler, python3 to build the fixture).  The
# reader is driven over the synthetic pass, and the colour and barcode parsers
# are checked on their own, because both accept forms (spaces in `rgb()`, the
# legacy singular `barcode`) that a naive parser would refuse.
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

if ! "$DIRECTORY_BUILD/pkpass-reader-test" "$FILE_PKPASS"; then
    fail_test "$NAME_TEST" 'pkpass-reader-test reported a failure'
fi

pass_test "$NAME_TEST"
