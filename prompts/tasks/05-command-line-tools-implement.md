# Task: Implement the Command-Line Tools feature

## TASK-DESCRIPTION
- Create: `sources/tools/pkpass_inspect.cpp`
- Create: `sources/tools/pkpass_render.cpp`
- Create: `sources/tools/pkpass_integrate.cpp`
- Create: `sources/tools/pkpass_viewer_ui.cpp`
- Create: `tests/60-command-line-tools.sh`
- Create: `scripts/program-build.sh`
- Create: `scripts/program-install.sh`
- Create: `scripts/release-stage.sh`
- Modify: `sources/CMakeLists.txt`
- Modify: `Makefile`

The tools implement the requirements in `prompts/features/06-command-line-tools.md`.
`pkpass-inspect` reports a pass and can print it as JSON; `pkpass-render` writes
the SVG card; `pkpass-integrate` manages the desktop handler and is what the
desktop entry runs.
`pkpass_viewer_ui.cpp` is the GTK4 window. It is built only where the GTK4
development files are present, which is why the CMake target is conditional and
why `pkpass-integrate handle` falls back to printed commands. On a host without
GTK4 the target is skipped and the rest of the project is unaffected.
`scripts/program-build.sh` writes the build tree to `dataflow.out/build/`, and
`scripts/release-stage.sh` stages that tree into the `bin/` and `share/` shape
`scripts/release-package.sh` packs.

## TASK-OUTPUT
Produce these complete files in this order, then the `VERIFICATION:` line:
1. `sources/tools/pkpass_inspect.cpp`
2. `sources/tools/pkpass_render.cpp`
3. `sources/tools/pkpass_integrate.cpp`
4. `sources/tools/pkpass_viewer_ui.cpp`
5. `tests/60-command-line-tools.sh`
6. `scripts/program-build.sh`
7. `scripts/program-install.sh`
8. `scripts/release-stage.sh`
9. `sources/CMakeLists.txt`
10. `Makefile`

## TASK-CONTEXT
<note>
`pkpass-inspect` exits zero on a mismatched manifest and reports the mismatch in
its output and in `verification.manifestOk`; the exit status is reserved for
being unable to read the file, so a caller can always read the report. The build
treats warnings as errors, so the viewer must compile cleanly against whichever
GTK4 major version is present.
</note>

## TASK-FILES

| Operation | Path |
|---|---|
| create | `sources/tools/pkpass_inspect.cpp` |
| create | `sources/tools/pkpass_render.cpp` |
| create | `sources/tools/pkpass_integrate.cpp` |
| create | `sources/tools/pkpass_viewer_ui.cpp` |
| create | `tests/60-command-line-tools.sh` |
| create | `scripts/program-build.sh` |
| create | `scripts/program-install.sh` |
| create | `scripts/release-stage.sh` |
| modify | `sources/CMakeLists.txt` |
| modify | `Makefile` |

## TASK-VERIFY
- Run: `make test` from the repository root.
- Expected: exit status 0, with `60-command-line-tools` reporting `ok`.

## TASK-FEATURES
- `prompts/features/06-command-line-tools.md`

## TASK-ACCEPTANCE
- `COMMAND-LINE-TOOLS-R001`
- `COMMAND-LINE-TOOLS-R002`
- `COMMAND-LINE-TOOLS-R004`
- `COMMAND-LINE-TOOLS-R005`
- `COMMAND-LINE-TOOLS-R009`
- `COMMAND-LINE-TOOLS-R011`
- `COMMAND-LINE-TOOLS-R012`
- `PKPASS-HANDLER-R017`

OUTPUT: the complete files in the order listed, ending with the `VERIFICATION:` line.
