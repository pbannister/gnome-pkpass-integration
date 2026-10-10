#!/bin/sh
#
# prompt-maturity.sh: score the machine-checkable part of the prompt-maturity
# rubric in documents/07-prompt-maturity.md.
#
# This is a self-assessment aid, not a gate. It measures what the repository can
# prove about itself; the dimensions it cannot prove are capped below the top
# score, and the cap is the finding. Read-only, and it never blocks on standard
# input (prompts/03-conventions.md section 7.1).
#
# Usage: prompt-maturity.sh [PROJECT_DIR]
set -eu

DIRECTORY_SCRIPT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "${1:-"$DIRECTORY_SCRIPT/.."}" && pwd)

has_file() {
    [ -f "$PROJECT_ROOT/$1" ]
}

has_text() {
    [ -f "$PROJECT_ROOT/$1" ] && grep -qF "$2" "$PROJECT_ROOT/$1"
}

count_files() {
    # count_files <glob-relative-pattern>
    set -- "$PROJECT_ROOT"/$1
    count=0
    for candidate in "$@"; do
        [ -f "$candidate" ] && count=$((count + 1))
    done
    echo "$count"
}

score=0
max=0

report() {
    # report <dimension> <score> <ceiling> <reason>
    score=$((score + $2))
    max=$((max + $3))
    printf '%-32s %s/%s  %s\n' "$1" "$2" "$3" "$4"
}

# Every dimension, its ceiling, and the evidence that earns the top score.
report 'entry point' \
    "$(if has_text tests/09-prompt-contract.sh 'AGENTS.md is missing'; then echo 3;
        elif has_file AGENTS.md; then echo 1; else echo 0; fi)" 3 \
    'AGENTS.md, checked by the validator'

if has_text prompts/02-workflow.md 'mandatory baseline'; then score_context=2; else score_context=1; fi
report 'context budget' "$score_context" 3 \
    'baseline plus task-relevant load; baseline size is not measured'

if has_text tests/09-prompt-contract.sh 'RULE_FAMILIES' && has_file tests/13-corpus-coverage.sh; then
    score_authority=3
else
    score_authority=2
fi
report 'authority' "$score_authority" 3 \
    'one home per rule; duplicates rejected; coverage enforced'

if has_text prompts/01-contract.md 'A file may be modified only when the task states the operation'; then
    score_scope=3
else
    score_scope=1
fi
report 'scope and authorization' "$score_scope" 3 \
    'operation lines plus the TASK-FILES agreement check'

if has_text Makefile 'test:' && has_text Makefile 'status:'; then score_loop=3; else score_loop=1; fi
report 'feedback loop' "$score_loop" 3 \
    'make test and make status'

if has_file TODO.md && has_file PHASES.md && has_file records/README.md; then
    score_memory=3
else
    score_memory=1
fi
report 'durable memory' "$score_memory" 3 \
    'TODO.md, PHASES.md, and the record forms'

if has_text prompts/01-contract.md 'as untrusted input' && has_file scripts/leak-gate.sh; then
    score_safety=2
else
    score_safety=1
fi
report 'safety' "$score_safety" 3 \
    'untrusted content and the leak gate; no injection corpus, no permission model'

count_eval=$(count_files 'evals/[0-9][0-9]-*.md')
if [ "$count_eval" -ge 3 ]; then score_eval=2; elif [ -d "$PROJECT_ROOT/evals" ]; then score_eval=1; else score_eval=0; fi
report 'behavioral evals' "$score_eval" 3 \
    "$count_eval case(s); manual and probabilistic, not measured automatically"

if has_file tests/13-corpus-coverage.sh; then score_discipline=3; else score_discipline=1; fi
report 'prompt-change discipline' "$score_discipline" 3 \
    'every validator check has a corpus fixture'

if has_text prompts/02-workflow.md 'its own git worktree'; then score_parallel=2; else score_parallel=1; fi
report 'parallel work' "$score_parallel" 3 \
    'worktree isolation; no delegation-depth rule'

echo "prompt-maturity: $score/$max"
echo 'prompt-maturity: a ceiling below 3 is the finding; see documents/07-prompt-maturity.md'
exit 0
