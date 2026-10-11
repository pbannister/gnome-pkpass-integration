#!/bin/sh
#
# QR encoder test (sources/qr/qr_encoder.cpp).
#
# Tier: tool-gated (cmake and a C++ compiler).  The encoder is checked against
# matrices produced by libqrencode, an independent implementation, so the test
# does not rest on this project's own arithmetic being right.
#
# tests/96-qr-decode.sh is the other half: it hands the rendered symbol to a
# decoder, where one is installed.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
build_program

if ! "$DIRECTORY_BUILD/qr-encoder-test"; then
    fail_test "$NAME_TEST" 'qr-encoder-test reported a failure'
fi

pass_test "$NAME_TEST"
