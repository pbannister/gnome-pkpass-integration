#!/bin/sh
#
# release-stage.sh: stage a built tree into the shape release-package.sh packs.
#
# scripts/release-package.sh (shared with the skeleton) packs a build tree that
# has bin/ and share/; CMake writes its programs to the build root, so this
# script copies them into bin/, the icon into share/, and the version the
# programs report into VERSION.  See prompts/03-conventions.md section 6.4.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPOSITORY_ROOT=$(CDPATH= cd -- "$DIRECTORY_SCRIPT/.." && pwd)
DIRECTORY_BUILD="$REPOSITORY_ROOT/dataflow.out/build"

if [ ! -x "$DIRECTORY_BUILD/pkpass-integrate" ]; then
    echo 'release-stage: no build; run make build first' >&2
    exit 1
fi

rm -rf "$DIRECTORY_BUILD/bin" "$DIRECTORY_BUILD/share"
mkdir -p "$DIRECTORY_BUILD/bin" "$DIRECTORY_BUILD/share/icons/hicolor/scalable/apps"

for program_name in pkpass-inspect pkpass-render pkpass-integrate pkpass-viewer; do
    if [ -x "$DIRECTORY_BUILD/$program_name" ]; then
        cp "$DIRECTORY_BUILD/$program_name" "$DIRECTORY_BUILD/bin/$program_name"
    fi
done

if [ -f "$DIRECTORY_BUILD/icons/pkpass-handler.svg" ]; then
    mkdir -p "$DIRECTORY_BUILD/bin/icons"
    cp "$DIRECTORY_BUILD/icons/pkpass-handler.svg" "$DIRECTORY_BUILD/bin/icons/pkpass-handler.svg"
    cp "$DIRECTORY_BUILD/icons/pkpass-handler.svg" \
        "$DIRECTORY_BUILD/share/icons/hicolor/scalable/apps/pkpass-handler.svg"
fi

"$DIRECTORY_BUILD/pkpass-integrate" --version | awk '{ print $2 }' > "$DIRECTORY_BUILD/VERSION"

echo "release-stage: staged $(ls "$DIRECTORY_BUILD/bin" | wc -l) program(s) into $DIRECTORY_BUILD/bin"
echo "release-stage: version $(cat "$DIRECTORY_BUILD/VERSION")"
