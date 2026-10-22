#!/bin/sh
#
# project_helpers.sh: this project's additions to tests/lib/test_helpers.sh.
#
# tests/lib/test_helpers.sh is vendored and must not be edited; it sources this
# file when present. These additions used to live inside the vendored file, where
# the next upgrade would have replaced them without a word.
set -eu

REPOSITORY_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DIRECTORY_BUILD="$REPOSITORY_ROOT/dataflow.out/build"
DIRECTORY_FIXTURES="$REPOSITORY_ROOT/dataflow.in/pkpass/sample"

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
