# Task: Document what is known and unknown about the format

## TASK-DESCRIPTION
- Create: `documents/10-pkpass-knowns-and-unknowns.md`
- Modify: `documents/README.md`
- Modify: `documents/08-pkpass-format.md`
- Modify: `README.md`
- Modify: `prompts/tasks/00-tasks.md`

The format reference (`documents/08-pkpass-format.md`) states what the format is.
This task adds the companion document that states how far each statement can be
trusted: what Apple documents, what has been observed directly, what is only
convention, what is unknown, what is not knowable without Apple hardware, and
which decisions this project made where the format is silent.

Every claim carries one of five labels — documented, observed, convention,
choice, or unknown — and the document is dated, because the format is
undocumented and moves.
It must not carry any value from a real pass; the synthetic fixture is the only
example material.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `documents/10-pkpass-knowns-and-unknowns.md`
2. `documents/README.md`
3. `documents/08-pkpass-format.md`
4. `README.md`
5. `prompts/tasks/00-tasks.md`

## TASK-CONTEXT
<note>
The two documents divide the work: `08` is the reference and lists the sources;
`10` is the epistemic companion and points at `08` for the detail rather than
repeating the field tables.
`documents/README.md` is append-only against the skeleton, so the new file is
listed in its project section.
</note>
<constraint>
No real pass, and no value taken from one, may appear in the document. Apple's
published examples and this repository's synthetic fixture are the only
permitted example values.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `documents/10-pkpass-knowns-and-unknowns.md` |
| modify | `documents/README.md` |
| modify | `documents/08-pkpass-format.md` |
| modify | `README.md` |
| modify | `prompts/tasks/00-tasks.md` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `09-prompt-contract` and `08-skeleton-diff` reporting `ok`, and `grep -c 'Unknown\|Observed\|Documented\|Convention\|Choice' documents/10-pkpass-knowns-and-unknowns.md` reporting a non-zero count.

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
