# Phases

Project work proceeds in numbered phases. A phase is a milestone that
groups one or more episodes (the reviewable work units; see
`prompts/how-to-write-episodes.md` §9 and the homelab project-pages
conventions `documents/09-project-pages-conventions.md` §6).

**Change the current phase only when committing the project** (owner rule
2026-08-26): the phase belongs to this project, not to the homelab
registry. The homelab reads it from the generated `site.out/phase.txt`
(emitted by `scripts/site-condense.sh` from the `Current:` line below) and
shows it next to the activity status (active/planned/deferred/complete),
which the human declares in the homelab registry.

Current: phase 4 — publish the pages and a release — started

- Phase 1 — Scaffold the project from the skeleton and adapt the rule corpus — complete
- Phase 2 — Read and check a pass: the container, `pass.json`, the manifest, and the signature — complete
- Phase 3 — Draw a pass and integrate it: the SVG card, the MIME handler, and the command-line tools — complete
- Phase 4 — Publish the pages and a release — started

States: `not-started` | `started` | `complete`. Keep this file in sync
with the episodes that advance each phase and with `TODO.md`.
