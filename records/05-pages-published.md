# Record: the project pages are published

## Outcome

The project is registered with the homelab and its pages are live at
<https://labs.bannister.us/projects/gnome-pkpass-integration/>.

The work is the homelab's registration commit `985151f` in the `00-homelab`
repository, plus this project's status update.

- The registration is one entry in the homelab's `sources/projects.yaml`:
  `id: gnome-pkpass-integration`, `status: active`, `visibility: public`,
  `self_published: true`, and a `pages_source` of `$HOME/work/gnome-pkpass-integration/site.out/`.
- The published tree is the standard page set (`index`, `dashboard`, `todo`,
  `prompts`, `documents`, `records`) plus the full-text pages for every `.md`
  under `prompts/`, `documents/`, and `records/` — 56 pages, normalized by the
  homelab's fetch.
- The project pushes nothing. The homelab reads `site.out/`, injects the one
  navigation bar, runs the sanitization gate, and publishes with its own deploy,
  as `documents/09-project-pages-conventions.md` requires.

## Decisions

- **`self_published`, not a site-generated page.** The project has its own page
  generator, so it owns its page content and the homelab only lists it and
  injects the navigation. The alternative — letting the site generate a page from
  the registry entry — would have replaced the generated status, dashboard, todo,
  and full-text pages with a summary and a README.
- **No `repository:` field, although the repository exists.** The registry
  refuses `repository` or `readme_source` on a `self_published` project, because
  the site would ignore them: the project owns its page and links nothing.
  The GitHub URL therefore does not appear on `/projects/` for this project.
  Changing that means either dropping `self_published` or relaxing the registry
  rule — a decision for the owner, not a side effect of registering.
- **The deploy publishes the whole site, not one project.** One publisher, one
  deploy: all twelve self-published trees were fetched and staged in the same
  run. That is the convention, and it means this project's publish also refreshed
  every other project's pages.
- **Registration is committed in the homelab, the publish is recorded here.**
  The registry belongs to the homelab's history; the outcome of publishing a
  project belongs to the project's records.

## Verification

- `sh tests/07-projects-registry.sh` in the homelab — 29 projects valid
  (25 public, 12 self-published), on 2026-10-10.
- `sh tests/06-project-pages.sh` in the homelab — this project reports OK twice:
  6 nav links with no depth sample, and "single homelab nav, standard page set,
  sanitized", on 2026-10-10.
- The fetch normalized 56 pages for this project and staged them under
  `/projects/gnome-pkpass-integration/`.
- `curl` after the deploy returns **200** for `/`, `dashboard.html`,
  `todo.html`, `prompts.html`, `documents.html`, `records.html`, and for the
  sampled full-text pages `documents-10-pkpass-knowns-and-unknowns.html`,
  `records-04-knowns-and-unknowns.html`, and
  `prompts-features-04-pass-renderer.html`, on 2026-10-10.
- The live index page carries the homelab nav once — a breadcrumb
  (`Labs home › Projects › GNOME PKPASS integration`) and the six page links,
  with `aria-current="page"` on Summary — and a real `<title>`, not the file
  name.
- `https://labs.bannister.us/projects/` lists the project and links to it.

## Commits

- `985151f` (in `00-homelab`) — projects map: register gnome-pkpass-integration.
  The registry entry.
- The status update to this project's `TODO.md` is its own commit.
- The commit that adds this record is separate, per `records/README.md`, and
  cites the homelab commit above rather than itself.

## Note on the environment

The deploy's `rsync -e ssh` needed a workaround: this container's
`/etc/ssh/ssh_config.d/20-systemd-ssh-proxy.conf` is owned by another user, which
OpenSSH 10 treats as fatal, so `ssh` exited 255 before connecting. Running the
deploy with a tiny `ssh` wrapper on `PATH` that adds `-F "$HOME/.ssh/config"`
made the connection and the deploy succeed.
That is an environment defect, not a project one, and it does not affect
`make test` here.
