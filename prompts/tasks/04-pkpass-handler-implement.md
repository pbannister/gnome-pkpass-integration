# Task: Implement the PKPASS Handler feature

## TASK-DESCRIPTION
- Create: `sources/integration/handler_integrator.h`
- Create: `sources/integration/handler_integrator.cpp`
- Create: `sources/tools/icons/pkpass-handler.svg`
- Create: `tests/handler_integrator_test.cpp`
- Create: `tests/90-handler-mime-definition.sh`
- Modify: `sources/CMakeLists.txt`

The integrator implements the requirements in `prompts/features/05-pkpass-handler.md`.
It writes the MIME definition, the handler entry, the three icons, and a record
under `$XDG_DATA_HOME`, makes the entry the default for both types with
`xdg-mime`, and reverses every step from the record.
A definition at the project's path that the project did not write is backed up
and restored byte for byte, because the machine may already define the type and
its definition is not this project's to lose.
The end-to-end test runs the whole install and uninstall inside a throwaway
`HOME`, with no display, so nothing touches the real desktop.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `sources/integration/handler_integrator.h`
2. `sources/integration/handler_integrator.cpp`
3. `sources/tools/icons/pkpass-handler.svg`
4. `tests/handler_integrator_test.cpp`
5. `tests/90-handler-mime-definition.sh`
6. `sources/CMakeLists.txt`

## TASK-CONTEXT
<note>
The record's shape follows the AppImage project's handler record: `key=value`
lines, a `previous_default=<type>` and a tab and the previous desktop id on one
line per MIME type, and `mime_created=1` only when this project created the
definition.
`shared-mime-info` 2.2 and later already define `application/vnd.apple.pkpass`,
so install over a machine that defines it must back the definition up rather
than assume it is absent. See `documents/09-mime-type-association.md`.
</note>
<constraint>
Nothing in this feature may write outside `$XDG_DATA_HOME`, `$XDG_CONFIG_HOME`,
the tool's own install prefix, and the temporary directory, and nothing may
require root.
</constraint>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `sources/integration/handler_integrator.h` |
| create | `sources/integration/handler_integrator.cpp` |
| create | `sources/tools/icons/pkpass-handler.svg` |
| create | `tests/handler_integrator_test.cpp` |
| create | `tests/90-handler-mime-definition.sh` |
| modify | `sources/CMakeLists.txt` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `90-handler-mime-definition` reporting `ok` and the four paths it asserts present during install.

## TASK-FEATURES
- `prompts/features/05-pkpass-handler.md`

## TASK-ACCEPTANCE
- `PKPASS-HANDLER-R001`
- `PKPASS-HANDLER-R003`
- `PKPASS-HANDLER-R004`
- `PKPASS-HANDLER-R005`
- `PKPASS-HANDLER-R009`
- `PKPASS-HANDLER-R011`
- `PKPASS-HANDLER-R013`
- `PKPASS-HANDLER-R014`
- `PKPASS-HANDLER-R016`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
