#!/bin/sh
#
# ZIP reader test (sources/zip/zip_reader.cpp).
#
# Tier: tool-gated (cmake, a C++ compiler, python3 to build the fixture).  A
# `.pkpass` is a ZIP whose members mix stored and deflated entries, so the test
# builds one from the fixture directory and reads it back through both the file
# and the in-memory entry points.
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

if ! "$DIRECTORY_BUILD/zip-reader-test" "$FILE_PKPASS"; then
    fail_test "$NAME_TEST" 'zip-reader-test reported a failure'
fi

pass_test "$NAME_TEST"
