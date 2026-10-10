#!/bin/sh
#
# program-build.sh: configure and build the C++ readers and tools.
#
# The build tree is generated output and lives in dataflow.out/build; the
# version header is generated there first, because the tools embed it.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPOSITORY_ROOT=$(CDPATH= cd -- "$DIRECTORY_SCRIPT/.." && pwd)
DIRECTORY_SOURCE="$REPOSITORY_ROOT/sources"
DIRECTORY_BUILD="$REPOSITORY_ROOT/dataflow.out/build"
BUILD_TYPE=${BUILD_TYPE:-Release}

mkdir -p "$DIRECTORY_BUILD"
sh "$DIRECTORY_SCRIPT/version-generate.sh"
cmake -S "$DIRECTORY_SOURCE" -B "$DIRECTORY_BUILD" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$DIRECTORY_BUILD" --parallel
