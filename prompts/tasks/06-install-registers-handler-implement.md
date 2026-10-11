# Task: Make the installer register the handler

## TASK-DESCRIPTION
- Modify: `scripts/program-install.sh`
- Create: `tests/91-install-handler.sh`
- Modify: `Makefile`
- Modify: `README.md`
- Modify: `TODO.md`
- Modify: `prompts/features/05-pkpass-handler.md`
- Modify: `prompts/tasks/00-tasks.md`

The installer implements the requirement in
`prompts/features/05-pkpass-handler.md` `PKPASS-HANDLER-R021`: installing the
tools also registers the handler, so a pass opens without a second command.

Registering must stay reversible and skippable: `PKPASS_INTEGRATION_NO_HANDLER=1`
installs the programs and registers nothing, and
`pkpass-integrate handler uninstall` reverses a registration at any time.
The registration runs the freshly installed program, so the desktop entry points
into the installation prefix rather than at the build tree.
A failure to register is reported, leaves the installed programs in place, and
makes the command exit nonzero, so a scripted install cannot believe it finished
when the association is missing.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `scripts/program-install.sh`
2. `tests/91-install-handler.sh`
3. `Makefile`
4. `README.md`
5. `TODO.md`
6. `prompts/features/05-pkpass-handler.md`
7. `prompts/tasks/00-tasks.md`

## TASK-CONTEXT
<note>
`program-install.sh` is what `make install` runs; `scripts/install.sh`, the
release installer, stays a separate entry point that a release tarball drives,
and it prints the integration step rather than performing it.
The installer is unattended, so the choice comes from an environment variable
and the failure is closed (`prompts/03-conventions.md` section 7.1).
See `documents/09-mime-type-association.md` for what the registration writes.
</note>
<constraint>
The test must point `HOME` and every `XDG_` path at a throwaway directory, so it
never registers the handler on the machine running the tests.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| modify | `scripts/program-install.sh` |
| create | `tests/91-install-handler.sh` |
| modify | `Makefile` |
| modify | `README.md` |
| modify | `TODO.md` |
| modify | `prompts/features/05-pkpass-handler.md` |
| modify | `prompts/tasks/00-tasks.md` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `91-install-handler` reporting `ok` and the pass type resolving to `pkpass-handler.desktop` inside the throwaway home.

## TASK-FEATURES
- `prompts/features/05-pkpass-handler.md`

## TASK-ACCEPTANCE
- `PKPASS-HANDLER-R021`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
