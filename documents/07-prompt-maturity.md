# Prompt Maturity

This document rates the prompt protocol against the practices that coding-agent
work has converged on. It is a self-assessment, not a gate.

## The Scale

Each dimension is scored 0 to 3. The level is about evidence, not intent.

- 0 — absent.
- 1 — documented as prose; a reader could follow it, nothing checks it.
- 2 — enforced mechanically; a script refuses a violation.
- 3 — enforced and measured; a regression is caught by a check that runs.

A dimension cannot reach 3 without a test that fails when the property breaks.
Prose that claims a property does not raise the score.

## The Dimensions

| Dimension | 3 requires | Typical 2 | Typical 1 |
|---|---|---|---|
| Entry point | a conventional entry file, checked by the validator | the file exists | described in the README |
| Context budget | the baseline size is measured and bounded | baseline plus task-relevant load | one undivided rules file |
| Authority | one home per rule, duplicates rejected, coverage enforced | duplicates rejected | rules restated in several files |
| Scope and authorization | operations enumerated and cross-checked | operations enumerated | scope described in prose |
| Feedback loop | one command, an observable result, and a report the agent cannot fake | the command exists | the command is documented |
| Durable memory | state files plus the record forms | some state files | conversation only |
| Safety | untrusted content handled and an injection corpus | untrusted content rules | rules only for secrets |
| Behavioral evals | a scored model run repeats on every prompt change | frozen cases, scored by hand | none |
| Prompt-change discipline | a rule change cannot land without its test | tests exist | review only |
| Parallel work | isolation plus the delegation authority of a subagent | isolation only | one writer assumed |

## Scoring This Project

Run `make maturity` (`sh scripts/prompt-maturity.sh`). It computes the
machine-checkable subset and prints each dimension with its ceiling. The script
is deliberately unable to award the top score for context budget, safety,
behavioral evals, and parallel work, because this project has no measurement for
them. The ceilings are the finding.

The manual remainder is judgment:

- Does the baseline load order match what a task actually needs? A rule that no
  task ever consults is context cost without benefit.
- Does an eval case fail for a reason the protocol predicted? A failure the
  protocol does not explain is a gap in the protocol, not only in the agent.

## Rules

- Raise a dimension by adding the missing evidence; never by editing a score.
- An eval pass raises confidence; it does not prove correctness. Prompts
  constrain probability, not behavior.
- Delete a rule that no longer changes behavior. Context cost is real, and a
  stronger model needs less of it.
- Add a rule only after a task fails without it; then add the check that would
  have caught the failure.
