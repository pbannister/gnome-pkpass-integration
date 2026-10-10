#!/bin/sh
#
# SHA-1 test (sources/crypto/sha1.cpp).
#
# Tier: tool-gated (cmake and a C++ compiler).  The digest algorithm is the one
# the pass manifest records, so it is checked against the published vectors and
# against its own streaming path before anything depends on it.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
build_program

if ! "$DIRECTORY_BUILD/sha1-test"; then
    fail_test "$NAME_TEST" 'sha1-test reported a failure'
fi

pass_test "$NAME_TEST"
