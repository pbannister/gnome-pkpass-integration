#!/bin/sh
#
# Installer integration test (prompts/features/05-pkpass-handler.md).
#
# Tier: tool-gated (cmake, a C++ compiler, xdg-mime).  `make install` must leave
# the desktop able to open a pass, so the installer's own behaviour is checked
# rather than only the handler command it calls:
#
#   * a plain install registers the handler, and the entry runs the installed
#     program rather than one in the build tree;
#   * both pass types then resolve to that entry;
#   * PKPASS_INTEGRATION_NO_HANDLER=1 installs the programs and registers
#     nothing;
#   * a registration that cannot happen leaves the programs installed and exits
#     nonzero, so a scripted install cannot believe it finished.
#
# Everything runs against a throwaway HOME and prefix, so the machine running
# the tests never has its own desktop changed.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
skip_unless_tool xdg-mime
build_program

DIRECTORY_TEST=$(mktemp -d)
trap 'rm -rf "$DIRECTORY_TEST"' EXIT HUP INT TERM

# run_install HOME PREFIX [ENV=value...]: run the installer with every desktop
# directory pointed into the throwaway home.  A caller-supplied variable comes
# last, so it overrides a default here.
run_install() {
    directory_home=$1
    directory_prefix=$2
    shift 2
    env -u DISPLAY -u WAYLAND_DISPLAY \
        HOME="$directory_home" \
        XDG_DATA_HOME="$directory_home/.local/share" \
        XDG_CONFIG_HOME="$directory_home/.config" \
        XDG_DATA_DIRS="$DIRECTORY_TEST/share" \
        PREFIX="$directory_prefix" \
        "$@" \
        sh "$REPOSITORY_ROOT/scripts/program-install.sh"
}

# in_sandbox HOME COMMAND...: run a command with the same throwaway desktop, so
# `xdg-mime` and the installed tool read the sandbox and not the real one.
in_sandbox() {
    directory_home=$1
    shift
    env -u DISPLAY -u WAYLAND_DISPLAY \
        HOME="$directory_home" \
        XDG_DATA_HOME="$directory_home/.local/share" \
        XDG_CONFIG_HOME="$directory_home/.config" \
        XDG_DATA_DIRS="$DIRECTORY_TEST/share" \
        "$@"
}

mkdir -p "$DIRECTORY_TEST/share"

# --- a plain install registers the handler -----------------------------------
DIRECTORY_HOME="$DIRECTORY_TEST/home-default"
DIRECTORY_PREFIX="$DIRECTORY_TEST/prefix-default"
mkdir -p "$DIRECTORY_HOME/.config"

FILE_LOG="$DIRECTORY_TEST/install.log"
if ! run_install "$DIRECTORY_HOME" "$DIRECTORY_PREFIX" >"$FILE_LOG" 2>&1; then
    echo "$NAME_TEST: the installer failed:" >&2
    sed 's/^/    /' "$FILE_LOG" >&2
    exit 1
fi

FILE_ENTRY="$DIRECTORY_HOME/.local/share/applications/pkpass-handler.desktop"
FILE_DEFINITION="$DIRECTORY_HOME/.local/share/mime/packages/pkpass.xml"
FILE_MANIFEST="$DIRECTORY_HOME/.local/share/gnome-pkpass-integration/pkpass-handler.manifest"
FILE_MIMEAPPS="$DIRECTORY_HOME/.config/mimeapps.list"

[ -x "$DIRECTORY_PREFIX/bin/pkpass-inspect" ] \
    || fail_test "$NAME_TEST" 'the installer did not install pkpass-inspect'
for file_expected in "$FILE_ENTRY" "$FILE_DEFINITION" "$FILE_MANIFEST" "$FILE_MIMEAPPS"; do
    [ -f "$file_expected" ] \
        || fail_test "$NAME_TEST" "the installer did not register the handler: $file_expected is missing"
done

# The entry must run the installed program, not the build tree: the build tree
# is a temporary directory that the next clean removes.
grep -qF "Exec=$DIRECTORY_PREFIX/bin/pkpass-integrate handle %f" "$FILE_ENTRY" \
    || fail_test "$NAME_TEST" "the entry does not run the installed program: $(grep '^Exec=' "$FILE_ENTRY")"

for type_name in application/vnd.apple.pkpass application/vnd.apple.pkpasses; do
    default_now=$(in_sandbox "$DIRECTORY_HOME" xdg-mime query default "$type_name")
    [ "$default_now" = 'pkpass-handler.desktop' ] \
        || fail_test "$NAME_TEST" "the installed handler is not the default for $type_name"
done

# An install is reversible, and the uninstall is the installed program's.
in_sandbox "$DIRECTORY_HOME" "$DIRECTORY_PREFIX/bin/pkpass-integrate" handler uninstall \
    >/dev/null 2>&1 || fail_test "$NAME_TEST" 'the installed program could not undo the registration'
if [ -e "$FILE_ENTRY" ]; then
    fail_test "$NAME_TEST" 'uninstall left the handler entry behind'
fi

# --- the opt-out installs the programs and registers nothing -----------------
DIRECTORY_HOME_OFF="$DIRECTORY_TEST/home-optout"
DIRECTORY_PREFIX_OFF="$DIRECTORY_TEST/prefix-optout"
mkdir -p "$DIRECTORY_HOME_OFF/.config"

run_install "$DIRECTORY_HOME_OFF" "$DIRECTORY_PREFIX_OFF" PKPASS_INTEGRATION_NO_HANDLER=1 \
    >"$DIRECTORY_TEST/optout.log" 2>&1 \
    || fail_test "$NAME_TEST" 'the opt-out install failed'

[ -x "$DIRECTORY_PREFIX_OFF/bin/pkpass-integrate" ] \
    || fail_test "$NAME_TEST" 'the opt-out install did not install the programs'
if [ -e "$DIRECTORY_HOME_OFF/.local/share/applications/pkpass-handler.desktop" ]; then
    fail_test "$NAME_TEST" 'the opt-out install registered the handler anyway'
fi

# --- a registration that cannot happen is reported and fails -----------------
# A file where the data directory belongs makes the registration impossible
# while leaving the prefix writable, which is the case this must survive.
DIRECTORY_HOME_BLOCKED="$DIRECTORY_TEST/home-blocked"
DIRECTORY_PREFIX_BLOCKED="$DIRECTORY_TEST/prefix-blocked"
mkdir -p "$DIRECTORY_HOME_BLOCKED/.config"
printf 'this is a file, not a directory\n' > "$DIRECTORY_TEST/blocked-data-home"

if run_install "$DIRECTORY_HOME_BLOCKED" "$DIRECTORY_PREFIX_BLOCKED" \
        XDG_DATA_HOME="$DIRECTORY_TEST/blocked-data-home" \
        >"$DIRECTORY_TEST/blocked.log" 2>&1; then
    fail_test "$NAME_TEST" 'the installer succeeded although the handler could not be registered'
fi

[ -x "$DIRECTORY_PREFIX_BLOCKED/bin/pkpass-inspect" ] \
    || fail_test "$NAME_TEST" 'a failed registration removed the installed programs'
grep -q 'NOT registered' "$DIRECTORY_TEST/blocked.log" \
    || fail_test "$NAME_TEST" 'the installer did not say that the handler was left unregistered'

pass_test "$NAME_TEST"
