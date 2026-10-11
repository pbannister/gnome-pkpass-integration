# Record: the installer registers the handler

## Outcome

`make install` no longer stops at installing the programs: it registers the
handler as well, so a downloaded pass opens as soon as the install is done.

The work is in commit `21f7df5`.

- `scripts/program-install.sh` ends by running the freshly installed
  `pkpass-integrate handler install`, after every program and the icon are in
  place.
- `PKPASS_INTEGRATION_NO_HANDLER=1` installs the programs and registers nothing.
- A registration that cannot happen leaves the installed programs in place,
  prints why, and exits nonzero.
- `tests/91-install-handler.sh` checks the plain install, the opt-out, and the
  failure path, all against a throwaway `HOME` and prefix.
- Feature `05-pkpass-handler.md` gains `PKPASS-HANDLER-R021`; task
  `06-install-registers-handler-implement.md` claims it.

`make test` is now 23 tests, all passing.

## Decisions

- **The owner's instruction overrides the earlier choice.** The project had made
  registration a separate, deliberate step on the reasoning that changing what
  opens a file type should be visible and reversible. The owner asked for the
  handler to be part of `make install`, and the reversibility is what carries
  the safety, not the separation: the record, the `uninstall`, and the opt-out
  are unchanged.
- **The registration runs the installed program.** The installer invokes
  `$PREFIX/bin/pkpass-integrate`, so the entry's `Exec` points into the
  installation prefix. Had the build tree's copy been run, the entry would have
  named a temporary directory that `make clean` removes.
- **The choice comes from the environment and the failure is closed.** The
  installer is unattended (`prompts/03-conventions.md` section 7.1), so the
  opt-out is `PKPASS_INTEGRATION_NO_HANDLER=1` rather than a question, and an
  install that cannot register exits nonzero after reporting that the programs
  are installed. Reporting success with the association missing is the failure
  this avoids.
- **`scripts/install.sh`, the release installer, is deliberately not changed.**
  The owner's instruction named `make install`. The release path takes a tarball
  and prints the integration step; making it register is a separate decision,
  and it is recorded in `TODO.md` as an open question rather than assumed here.

## Verification

- `make test` — 23 tests, all pass, on 2026-10-10. `91-install-handler` is the
  new one.
- The installer was run by hand against a throwaway `HOME` and prefix, on
  2026-10-10, and left both types resolving to `pkpass-handler.desktop` with
  the definition, three icons, and the record in place; the opt-out left the
  programs installed and the desktop untouched.
- The test sandboxes `HOME`, `XDG_DATA_HOME`, `XDG_CONFIG_HOME`, and
  `XDG_DATA_DIRS`, so the machine running the tests never has its own desktop
  changed. The real `$HOME/.local` was not written to during this work.

## Commits

- `21f7df5` — feat: register the handler from make install. The installer, the
  test, the Makefile comment, the README, `TODO.md`, the feature requirement,
  and the new task.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the work commit above rather than itself.
