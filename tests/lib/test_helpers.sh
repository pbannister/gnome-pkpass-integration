#!/bin/sh
#
# test_helpers.sh: shared definitions for the tests in tests/.
#
# This file is sourced, not executed: scripts/tests-run.sh executes only
# tests/*.sh, so a helper under tests/lib/ never runs on its own. A test uses
# it with:
#
#     . "$(dirname -- "$0")/lib/test_helpers.sh"
#
# The standard tool-gated skip message is "SKIP: missing tool: <tool>", so a
# skip is visible in the run transcript without being a failure.

# REPOSITORY_ROOT resolves through the sourcing script's own path, so a test may
# call a helper from any working directory.
REPOSITORY_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DIRECTORY_BUILD="$REPOSITORY_ROOT/dataflow.out/build"
DIRECTORY_FIXTURES="$REPOSITORY_ROOT/dataflow.in/pkpass/sample"

# skip_test MESSAGE: report a clean skip and exit 0.
skip_test() {
    echo "SKIP: $1"
    exit 0
}

# skip_unless_tool TOOL: skip cleanly when TOOL is not on PATH.
skip_unless_tool() {
    command -v "$1" >/dev/null 2>&1 || skip_test "missing tool: $1"
}

# fail_test TEST_NAME MESSAGE: report a failure and exit 1.
fail_test() {
    echo "$1: $2" >&2
    exit 1
}

# pass_test TEST_NAME: report success and exit 0.
pass_test() {
    echo "$1: ok"
    exit 0
}

# build_program: configure and build the C++ readers and tools.  A test that
# needs the reader calls this rather than assuming an earlier test built it,
# because each test must also pass when it is run on its own.
build_program() {
    sh "$REPOSITORY_ROOT/scripts/program-build.sh" >/dev/null
}

# build_synthetic_pkpass SOURCE_DIR OUTPUT_FILE: zip a directory of pass members
# into a `.pkpass`, storing the PNGs and deflating the JSON the way Apple's own
# builds do.  The manifest inside the source directory then describes files whose
# bytes the archive does not change, which is what makes the digest check
# meaningful.
build_synthetic_pkpass() {
    directory_source=$1
    file_output=$2
    python3 - "$directory_source" "$file_output" <<'PYTHON_BUILD'
import os
import sys
import zipfile

directory_source, file_output = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(file_output, "w") as archive:
    for name in sorted(os.listdir(directory_source)):
        path = os.path.join(directory_source, name)
        if not os.path.isfile(path):
            continue
        method = zipfile.ZIP_STORED if name.endswith(".png") else zipfile.ZIP_DEFLATED
        archive.write(path, name, compress_type=method)
PYTHON_BUILD
}
