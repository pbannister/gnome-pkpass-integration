#!/bin/sh
#
# program-install.sh: install the built tools and the icon into a prefix.
#
# The default prefix is $HOME/.local, so no root access is required.  The
# installed path is what the desktop entry runs, so it must be stable; the
# handler is registered separately, by `pkpass-integrate handler install`, which
# keeps system integration a deliberate step.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPOSITORY_ROOT=$(CDPATH= cd -- "$DIRECTORY_SCRIPT/.." && pwd)
DIRECTORY_BUILD="$REPOSITORY_ROOT/dataflow.out/build"
PREFIX=${PREFIX:-$HOME/.local}
DIRECTORY_BIN="$PREFIX/bin"

mkdir -p "$DIRECTORY_BIN"

# Copy to a temporary name and rename it over the destination: writing in place
# fails with "Text file busy" while the program is running, and a rename leaves
# any running copy with the file it started from.
install_program() { # <source> <destination>
    file_temporary="$2.installing.$$"
    cp "$1" "$file_temporary"
    chmod 755 "$file_temporary"
    mv -f "$file_temporary" "$2"
    echo "program-install: $2"
}

for program_name in pkpass-inspect pkpass-render pkpass-integrate; do
    if [ ! -x "$DIRECTORY_BUILD/$program_name" ]; then
        echo "program-install: missing $DIRECTORY_BUILD/$program_name; run 'make build' first" >&2
        exit 1
    fi
    install_program "$DIRECTORY_BUILD/$program_name" "$DIRECTORY_BIN/$program_name"
done

# The graphical viewer sits next to the tool so the tool can exec it.  It is a
# GTK4 program, built only where the GTK4 development files are present.
if [ -x "$DIRECTORY_BUILD/pkpass-viewer" ]; then
    install_program "$DIRECTORY_BUILD/pkpass-viewer" "$DIRECTORY_BIN/pkpass-viewer"
else
    echo "program-install: pkpass-viewer was not built; the handler prints a report instead" >&2
fi

# The icon, both next to the tool (where the handler looks for it) and in the
# icon theme (where the desktop looks for it).
DIRECTORY_ICON="$DIRECTORY_BUILD/icons"
if [ -f "$DIRECTORY_ICON/pkpass-handler.svg" ]; then
    mkdir -p "$DIRECTORY_BIN/icons"
    cp "$DIRECTORY_ICON/pkpass-handler.svg" "$DIRECTORY_BIN/icons/pkpass-handler.svg"
    DIRECTORY_THEME="$PREFIX/share/icons/hicolor/scalable/apps"
    mkdir -p "$DIRECTORY_THEME"
    cp "$DIRECTORY_ICON/pkpass-handler.svg" "$DIRECTORY_THEME/pkpass-handler.svg"
    chmod 644 "$DIRECTORY_THEME/pkpass-handler.svg"
    echo "program-install: $DIRECTORY_THEME/pkpass-handler.svg"
    # A stale cache hides the icon from GTK, which trusts a cache that is not
    # older than the theme directory and then never rescans it.
    if command -v gtk4-update-icon-cache >/dev/null 2>&1; then
        gtk4-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" >/dev/null 2>&1 || true
    elif command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" >/dev/null 2>&1 || true
    fi
fi

echo "program-install: register the handler with: pkpass-integrate handler install"
