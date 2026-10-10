#!/bin/sh
#
# Phase-parser test (scripts/site-condense.sh, phase.txt output).
#
# Tier: portable. The parser must read the project phase from PHASES.md even
# when the "Current:" line carries a description, must validate the state
# against the three allowed values, and must leave phase.txt unwritten (not
# wrong) when the line does not fit.
#
# The test builds a throwaway repository around a copy of the condenser.
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPOSITORY_ROOT=$(CDPATH= cd -- "$DIRECTORY_SCRIPT/.." && pwd)

DIRECTORY_TEST=$(mktemp -d)
trap 'rm -rf "$DIRECTORY_TEST"' EXIT HUP INT TERM

fail() {
    echo "04-phase-parse: $1" >&2
    exit 1
}

# condense PHASES_LINE: run the condenser in a fresh throwaway repo whose
# PHASES.md carries PHASES_LINE, and leave the repo at $DIRECTORY_TEST/run.
condense() {
    rm -rf "$DIRECTORY_TEST/run"
    mkdir -p "$DIRECTORY_TEST/run/scripts" "$DIRECTORY_TEST/run/site.in" \
        "$DIRECTORY_TEST/run/prompts" "$DIRECTORY_TEST/run/documents" "$DIRECTORY_TEST/run/records"
    cp "$REPOSITORY_ROOT/scripts/site-condense.sh" "$DIRECTORY_TEST/run/scripts/site-condense.sh"
    cp "$REPOSITORY_ROOT/site.in/template.html" "$DIRECTORY_TEST/run/site.in/template.html"
    printf '%s\n' '# TODO' > "$DIRECTORY_TEST/run/TODO.md"
    {
        echo '# Phases'
        echo
        echo "$1"
    } > "$DIRECTORY_TEST/run/PHASES.md"
    (cd "$DIRECTORY_TEST/run" && sh scripts/site-condense.sh >/dev/null 2>&1)
}

# A description in the middle is allowed; the state is the last field.
condense 'Current: phase 3 — vendor selection — started'
grep -q '^PHASE=3$' "$DIRECTORY_TEST/run/site.out/phase.txt" \
    || fail 'a descriptive Current line did not parse the phase number'
grep -q '^PHASE_STATE=started$' "$DIRECTORY_TEST/run/site.out/phase.txt" \
    || fail 'a descriptive Current line did not parse the state'

# The short form still works.
condense 'Current: phase 1 — not-started'
grep -q '^PHASE=1$' "$DIRECTORY_TEST/run/site.out/phase.txt" \
    || fail 'the short Current form did not parse'
grep -q '^PHASE_STATE=not-started$' "$DIRECTORY_TEST/run/site.out/phase.txt" \
    || fail 'the short Current form parsed the wrong state'

# An unknown state leaves phase.txt unwritten rather than emitting a wrong
# phase.
condense 'Current: phase 2 — in-progress'
if [ -f "$DIRECTORY_TEST/run/site.out/phase.txt" ]; then
    fail 'an unknown state wrote phase.txt'
fi

echo '04-phase-parse: ok'
exit 0
