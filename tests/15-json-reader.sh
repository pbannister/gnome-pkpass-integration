#!/bin/sh
#
# JSON reader test (sources/json/json_reader.cpp).
#
# Tier: tool-gated (cmake and a C++ compiler).  `pass.json` and `manifest.json`
# are both JSON, so the reader's handling of escapes, numbers, empty containers,
# and malformed input is checked directly.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
build_program

if ! "$DIRECTORY_BUILD/json-reader-test"; then
    fail_test "$NAME_TEST" 'json-reader-test reported a failure'
fi

pass_test "$NAME_TEST"
