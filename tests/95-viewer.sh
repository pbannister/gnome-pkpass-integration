#!/bin/sh
#
# GTK4 viewer test (prompts/features/06-command-line-tools.md).
#
# Tier: tool-gated (cmake, a C++ compiler, python3, Xvfb, and x11-utils).  The
# shell tests otherwise drive the library and the command-line tools; this one
# proves the graphical half actually opens a window, because a viewer that
# compiles and then exits is the failure this project has no other way to catch.
#
# The window is checked through the X server's own window tree rather than by
# looking at a screenshot: the title and the WM class are exactly what the
# desktop uses to match a running window to the handler entry.
#
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$DIRECTORY_SCRIPT/lib/test_helpers.sh"

NAME_TEST=$(basename "$0" .sh)
skip_unless_tool cmake
skip_unless_tool python3
skip_unless_tool Xvfb
skip_unless_tool xwininfo
build_program

# The viewer is built only where the GTK4 development files are present, so its
# absence is a clean skip rather than a failure.
if [ ! -x "$DIRECTORY_BUILD/pkpass-viewer" ]; then
    skip_test 'missing built tool: pkpass-viewer (install the GTK4 development files)'
fi

DIRECTORY_TEMPORARY=$(mktemp -d)
PID_DISPLAY=
PID_VIEWER=
cleanup() {
    if [ -n "$PID_VIEWER" ]; then
        kill "$PID_VIEWER" 2>/dev/null || true
    fi
    if [ -n "$PID_DISPLAY" ]; then
        kill "$PID_DISPLAY" 2>/dev/null || true
    fi
    rm -rf "$DIRECTORY_TEMPORARY"
}
trap cleanup EXIT HUP INT TERM

FILE_PKPASS="$DIRECTORY_TEMPORARY/sample.pkpass"
build_synthetic_pkpass "$DIRECTORY_FIXTURES" "$FILE_PKPASS"

# A private display, so the test never touches a real session.  The number is
# derived from the process id to avoid colliding with a concurrent run.
DISPLAY_NUMBER=$(( 90 + ($$ % 8) ))
Xvfb ":$DISPLAY_NUMBER" -screen 0 1024x768x24 >/dev/null 2>&1 &
PID_DISPLAY=$!

count_wait=0
while [ "$count_wait" -lt 20 ]; do
    if DISPLAY=":$DISPLAY_NUMBER" xdpyinfo >/dev/null 2>&1; then
        break
    fi
    sleep 0.25
    count_wait=$((count_wait + 1))
done
if ! DISPLAY=":$DISPLAY_NUMBER" xdpyinfo >/dev/null 2>&1; then
    fail_test "$NAME_TEST" 'the private X display did not start'
fi

# open_window: start a command on the private display and check that a window
# with the expected title appears and that the process is still alive.
# Usage: open_window <expected-title-fragment> <command...>
open_window() {
    text_expected=$1
    shift
    DISPLAY=":$DISPLAY_NUMBER" GSK_RENDERER=cairo GDK_BACKEND=x11 "$@" \
        >"$DIRECTORY_TEMPORARY/window.log" 2>&1 &
    PID_VIEWER=$!
    sleep 2

    if ! kill -0 "$PID_VIEWER" 2>/dev/null; then
        sed 's/^/    /' "$DIRECTORY_TEMPORARY/window.log" | tail -5 >&2
        fail_test "$NAME_TEST" "the process exited instead of showing a window: $*"
    fi

    text_tree=$(DISPLAY=":$DISPLAY_NUMBER" xwininfo -root -tree 2>/dev/null || true)
    printf '%s\n' "$text_tree" | grep -q "$text_expected" \
        || fail_test "$NAME_TEST" "no window titled '$text_expected' appeared"
    printf '%s\n' "$text_tree" | grep -q 'pkpass-viewer' \
        || fail_test "$NAME_TEST" 'the window does not carry the pkpass-viewer WM class'

    kill "$PID_VIEWER" 2>/dev/null || true
    wait "$PID_VIEWER" 2>/dev/null || true
    PID_VIEWER=
}

# The window is titled with the pass description, and its WM class is the value
# the handler entry names in StartupWMClass, which is what the dock matches on.
open_window 'Example Wallet Pass' "$DIRECTORY_BUILD/pkpass-viewer" "$FILE_PKPASS"

# The desktop entry point must put the same window up; this is what a
# double-clicked pass runs.
open_window 'Example Wallet Pass' "$DIRECTORY_BUILD/pkpass-integrate" handle "$FILE_PKPASS"

# A file that is not a pass must still explain itself in a window rather than
# failing silently.
open_window 'Pass' "$DIRECTORY_BUILD/pkpass-viewer" "$DIRECTORY_TEMPORARY/absent.pkpass"

pass_test "$NAME_TEST"
