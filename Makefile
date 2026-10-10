#
#	Makefile for the GNOME PKPASS Integration project.
#
#	This Makefile is the human-facing driver.
#	Each rule calls the appropriate tool instead of reimplementing work:
#		all:    the project's primary artifact (the programs)
#		build:  sh scripts/program-build.sh (readers, renderer, and tools)
#		        The build tree is written to dataflow.out/build/.
#		site:   sh scripts/site-state-fetch.sh (live state, owning host)
#		        sh scripts/site-build.sh + sh scripts/site-condense.sh
#		        then sh scripts/leak-gate.sh site.out (the project-side
#		        sanitization check; the homelab re-runs its own at publish)
#		        (the standard page set; see prompts/features/02-project-pages.md)
#		state:  sh scripts/site-state-fetch.sh (refresh the live-state file only)
#		status: sh scripts/status.sh (phase, next TODO, last test run, tree)
#		maturity: sh scripts/prompt-maturity.sh (the prompt rubric, self-assessment)
#		check:  sh scripts/leak-gate.sh site.out (the gate alone)
#		clean:  remove generated output, including the path-bound build tree
#		test:   npm test
#		deploy: RETIRED - all publishing to labs.bannister.us goes through
#		        the homelab project (homelab-publish; see the homelab's
#		        documents/09-project-pages-conventions.md). This target
#		        only reminds you of that; use the homelab's make deploy.
#		install: sh scripts/program-install.sh (prefix defaults to $HOME/.local)
#		release: build, stage into bin/, check the version names this commit,
#		        then sh scripts/release-package.sh (the files a release
#		        publishes)
#

all: build

build:
	sh scripts/program-build.sh

state:
	sh scripts/site-state-fetch.sh

status:
	sh scripts/status.sh

maturity:
	sh scripts/prompt-maturity.sh

site:
	sh scripts/site-state-fetch.sh
	sh scripts/site-build.sh
	sh scripts/site-condense.sh
	sh scripts/leak-gate.sh site.out

check:
	sh scripts/leak-gate.sh site.out

clean:
	rm -rf dataflow.out/build dataflow.out/release dataflow.out/generated
	rm -f dataflow.out/* site.out/* logs/*

test:
	npm test

deploy:
	@echo '==== RETIRED: publishing goes through the homelab project (homelab-publish).'
	@echo '==== Build the pages here (make site), then run the homelab''s "make deploy"'
	@echo '==== to publish (see homelab documents/09-project-pages-conventions.md).'

install:
	sh scripts/program-install.sh

release:
	sh scripts/program-build.sh
	sh scripts/release-stage.sh
	sh scripts/release-gate.sh "$$(cat dataflow.out/build/VERSION)"
	sh scripts/release-package.sh

.PHONY: all build site state status maturity check clean test deploy install release
