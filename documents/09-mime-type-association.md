# MIME Type Association

A desktop answers one question when a person opens a `.pkpass` file: which application handles this?
The answer comes from two independent pieces of state.
One is a MIME type definition that says a `.pkpass` file is an Apple Wallet pass, and the other is a default-application association that says which program opens that type.
This document describes both layers of the freedesktop system, the two types involved, and exactly what the `gnome-pkpass-integration` project writes to become that application.

## The Freedesktop MIME System

### The Compiled Database

The system-wide database lives in `/usr/share/mime`.
Its XML sources are the package files under `/usr/share/mime/packages/`.
`update-mime-database DIRECTORY` compiles those sources into the directory: `mime.cache` holds the type, glob, magic, and alias indexes, and the text files `globs`, `magic`, `subclasses`, `generic-icons`, and `icons` sit beside it.
A definition XML file therefore changes nothing until `update-mime-database` runs.
GIO, the GNOME I/O layer, reads this compiled database for a file's content type, which is why a GNOME application can see a pass type while `file` sees only a ZIP.

### Per-User Overrides

A user's database lives under `$XDG_DATA_HOME/mime`, which defaults to `$HOME/.local/share/mime`.
It has the same layout: XML under `mime/packages/` and a compiled `mime.cache` beside it.
The user database is searched before the system database, so a per-user definition of a type outranks the system definition of the same type.
A per-user install needs no root and leaves `/usr/share` untouched.

### Application Associations

Application associations are a separate mechanism from the type database.
They live in `mimeapps.list` files, whose three relevant groups are:

- `[Default Applications]`, which maps a MIME type to the one desktop-file ID that opens it.
- `[Added Associations]`, which lists further desktop-file IDs that can open the type.
- `[Removed Associations]`, which removes a desktop-file ID from a type's list.

The desktop-specific file has the highest precedence, so `$XDG_CONFIG_HOME/<desktop>-mimeapps.list` overrides `$XDG_CONFIG_HOME/mimeapps.list`, which defaults to `$HOME/.config/mimeapps.list`.
`xdg-mime default` writes into the user's `$XDG_CONFIG_HOME/mimeapps.list`.

### The Application Cache

`update-desktop-database DIRECTORY` scans the `.desktop` files in an applications directory and writes `mimeinfo.cache` beside them.
The cache maps each MIME type to the desktop-file IDs that declare it in `MimeType=`.
GIO reads this cache when it is asked which applications implement a type, so a newly installed entry can stay invisible to a file manager until the cache is rebuilt.
The per-user applications directory is `$XDG_DATA_HOME/applications`, which defaults to `$HOME/.local/share/applications`.

### Querying and Setting the Default

Two `xdg-mime` commands cover the read and write sides of an association.

- `xdg-mime query default TYPE` prints the desktop-file ID recorded as the default for the type, and prints nothing when no default is recorded.
- `xdg-mime default FILE.desktop TYPE...` records one desktop file as the default for one or more types.

## The Two Types

### application/vnd.apple.pkpass

`application/vnd.apple.pkpass` is the type of a single pass, and its glob pattern is `*.pkpass`.
shared-mime-info has shipped this definition since version 2.2.

### application/vnd.apple.pkpasses

`application/vnd.apple.pkpasses` is the type of a bundle, which is a ZIP holding several `.pkpass` files, and its glob pattern is `*.pkpasses`.
shared-mime-info has shipped this definition since version 2.5.
A distribution with shared-mime-info older than 2.2 has no definition for either type, and one older than 2.5 has no definition for the bundle.
On such a system a `.pkpass` file resolves to `application/zip`, or to `application/octet-stream` when the name gives no other clue.

### Registration Status

Neither type is registered with IANA.
The IANA application registry carries other Apple vendor types, such as `application/vnd.apple.installer` and `application/vnd.apple.mpegurl`, but no pass type.
Both types are conventions that Apple documents and that format databases have adopted, not standards.

### A Pass Is a ZIP

A `.pkpass` file is a ZIP archive whose extension was changed from `.zip`.
`file --mime-type` reports `application/zip` for it, because libmagic has no rule for the pass format.
That answer is expected and is not evidence that a MIME registration failed.
The GLib answer and the libmagic answer come from different databases.

## The Magic Rule and Its Caveat

Both definitions carry a magic rule, and both rules test the first bytes of the file.

- For `application/vnd.apple.pkpass` the rule matches the four bytes `PK\003\004` at offset 0, then the string `pass.json` at offset 30.
- For `application/vnd.apple.pkpasses` the rule matches `PK\003\004` at offset 0, then the string `.pkpass` in the byte range that begins at offset 30, written `offset="30:256"`.

Offset 30 is where the ZIP local file header's file-name field begins.
The fixed part of that header is 30 bytes: 4 of signature, 2 of version needed, 2 of flags, 2 of compression method, 2 of modification time, 2 of modification date, 4 of CRC-32, 4 of compressed size, 4 of uncompressed size, 2 of file-name length, and 2 of extra-field length.
The nested match is conjunctive, so the single-pass rule fires only when the first ZIP entry's name begins with `pass.json` at byte 30.
The file-name field always begins at offset 30, whatever the name's length and whatever the entry's compression method, so the test is on the bytes of that name.
A first entry named `manifest.json` fails the rule, and because the comparison is a string match at a fixed offset, a first entry named `pass.json.bak` satisfies it.
The bundle rule is looser: it accepts `.pkpass` anywhere in the scanned range, which covers a first entry named for an individual pass.

Detection therefore rests on the glob for most real passes.
Any ZIP named `*.pkpass` types as `application/vnd.apple.pkpass` even when it contains no pass at all, and a pass that fails the magic rule still types as one because of its name.
The MIME type is a hint about the name and the first bytes, not a validated claim about the contents.
A reader must open the archive and check for `pass.json`, `manifest.json`, and `signature` rather than trust the type it was handed.

## What This Project Installs

The integration writes only inside the user's own XDG directories.
Every path is resolved once from the environment, with `XDG_DATA_HOME` defaulting to `$HOME/.local/share`.

### Paths

- `$XDG_DATA_HOME/mime/packages/pkpass.xml` — the MIME definition, inside the `$XDG_DATA_HOME/mime` directory passed to `update-mime-database`.
- `$XDG_DATA_HOME/applications/pkpass-handler.desktop` — the handler entry.
- `$XDG_DATA_HOME/icons/hicolor/scalable/apps/pkpass-handler.svg` — the handler's own icon.
- `$XDG_DATA_HOME/icons/hicolor/scalable/mimetypes/application-vnd.apple.pkpass.svg` — the single-pass type icon.
- `$XDG_DATA_HOME/icons/hicolor/scalable/mimetypes/application-vnd.apple.pkpasses.svg` — the bundle type icon.
- `$XDG_DATA_HOME/gnome-pkpass-integration/pkpass-handler.manifest` — the record of the install.
- `$XDG_DATA_HOME/gnome-pkpass-integration/backup/pkpass.xml` — the backup of a definition this project did not write.

### The MIME Definition

The function `mime_definition_xml()` returns the exact text written to `pkpass.xml`.
It declares two `mime-type` elements inside a `mime-info` document whose namespace is `http://www.freedesktop.org/standards/shared-mime-info`.
The single-pass element carries these values:

- `type="application/vnd.apple.pkpass"`.
- `<comment>Apple Wallet pass</comment>`.
- `<sub-class-of type="application/zip"/>`.
- `<generic-icon name="pkpass-handler"/>`.
- `<magic priority="65">` with `<match type="string" value="PK\003\004" offset="0">` containing `<match type="string" value="pass.json" offset="30"/>`.
- `<glob pattern="*.pkpass"/>`.

The bundle element differs in four values:

- `type="application/vnd.apple.pkpasses"`.
- `<comment>Apple Wallet passes bundle</comment>`.
- the nested match is `<match type="string" value=".pkpass" offset="30:256"/>`.
- `<glob pattern="*.pkpasses"/>`.

Both elements share `sub-class-of application/zip`, `generic-icon pkpass-handler`, and magic priority 65.
The priority raises the rules above the database's default, so a file whose bytes satisfy them is typed as a pass rather than as a plain ZIP, and the generic icon is declared on each type so neither falls back to the parent ZIP icon.

### The Handler Entry

The function `handler_desktop_text()` returns the exact text written to `pkpass-handler.desktop`.
The entry has these keys, in this order:

- `[Desktop Entry]`, the group header.
- `Type=Application`.
- `Name=PKPASS Viewer`.
- `Comment=Inspect and display an Apple Wallet pass`.
- `Exec=<tool> handle %f`, with the resolved tool path, the `handle` subcommand, and the single-file placeholder.
- `Icon=pkpass-handler`.
- `StartupWMClass=pkpass-viewer`.
- `Terminal=false`.
- `NoDisplay=true`, which keeps the entry out of the application menu while it still serves as an association target.
- `MimeType=application/vnd.apple.pkpass;application/vnd.apple.pkpasses;`.
- `X-Integrated-By=gnome-pkpass-integration`.
- `X-Integrated-At=<version>`, the version string of the build that ran the install.

The tool path is the installed copy at `$HOME/.local/bin/pkpass-integrate` when that file exists, otherwise the running executable resolved through `/proc/self/exe`, and otherwise the bare name `pkpass-integrate`.
A path containing a space, a tab, or one of `"`, `'`, `\`, `$`, or a backtick is wrapped in double quotes, and the double quote, backslash, dollar, and backtick characters are escaped inside the quotes.
The `%f` placeholder passes exactly one file, which is what the `handle` subcommand accepts.

### The Icons

The install copies one SVG, taken from the first of three candidate locations that exists: beside the tool as `icons/pkpass-handler.svg`, beside the tool as `pkpass-handler.svg`, and in the install prefix at `share/icons/hicolor/scalable/apps/pkpass-handler.svg`.
When no candidate exists, the install logs that no icon source was found and writes no icon.
The same bytes are otherwise written to three names:

- `hicolor/scalable/apps/pkpass-handler.svg`, the icon named by `Icon=pkpass-handler`.
- `hicolor/scalable/mimetypes/application-vnd.apple.pkpass.svg`, the single-pass type icon.
- `hicolor/scalable/mimetypes/application-vnd.apple.pkpasses.svg`, the bundle type icon.

The type file names come from the MIME type with `/` replaced by `-`.
After the icons are written, the install refreshes the icon cache with `gtk4-update-icon-cache -f -t <hicolor directory>`, or with `gtk-update-icon-cache -f -t` as a fallback.
GTK trusts a cache that is not older than the theme directory and never rescans a current one, so a stale cache hides every newly installed icon.
A missing cache program is not an error.

### The Record

`pkpass-handler.manifest` is a flat text file of `key=value` lines, read back by splitting each line at its first `=`.
It can hold these keys:

- `handler_desktop`, the absolute path of the desktop entry.
- `icon`, one line per installed icon, so three lines when an icon source was found and none when it was not.
- `mime_package`, the absolute path of the definition, `$XDG_DATA_HOME/mime/packages/pkpass.xml`.
- `mime_backup`, the absolute path of the backup, or an empty value when no definition was displaced.
- `mime_created`, written as `mime_created=1` only when this install created the definition, and absent when it reused or replaced one.
- `previous_default`, one line per handled type, holding the type, a tab, and the desktop-file ID that was the default before the install.

Uninstall requires this record, and the record is the last file an install writes, so an install that fails earlier leaves no record of a partial state.

## Design Decisions

### Why Write a Definition Where shared-mime-info Already Has One

A current distribution already defines both types, so the file is not needed to make the types exist there.
The project writes it for two reasons.
The `generic-icon` element can attach this project's icon only from a definition that carries it, and the system definition in `/usr/share` is not the project's to edit.
A per-user definition of a type outranks the system definition of the same type, so the user-level file is the supported place to change the icon.
On a distribution with shared-mime-info older than 2.2 or 2.5, the same file is also what makes the types exist at all.

### Why Back Up and Restore a Foreign Definition

The file `$XDG_DATA_HOME/mime/packages/pkpass.xml` may have been written by another program, and deleting it would break that program.
The install therefore copies a definition it did not write to `gnome-pkpass-integration/backup/pkpass.xml` before replacing it, and uninstall copies the backup back.
A definition this project created itself has no backup, and uninstall removes it.
The distinction is recorded as `mime_created=1` with an empty `mime_backup`.
An existing backup is reused rather than overwritten, so the earliest displaced definition survives repeated installs.

### Why Read and Record the Previous Default

`xdg-mime default` writes a new association and does not report the old one.
The install therefore reads `xdg-mime query default TYPE` for each type first and stores the answer in `previous_default` lines.
Uninstall replays those values with `xdg-mime default` and skips a type whose recorded value is empty, so a type that had no default loses the association instead of gaining a wrong one.
When the query already reports this project's own entry, a value recorded by an earlier run wins, so reinstalling does not overwrite the real previous handler with the project's own desktop-file ID.

### Why the Type Icon Uses the Derived Name

GIO derives a file's icon name from the MIME type by replacing `/` with `-` and looking in the theme's `mimetypes` directory.
`application/vnd.apple.pkpass` therefore needs `hicolor/scalable/mimetypes/application-vnd.apple.pkpass.svg` for a file manager to show a pass icon.
The `generic-icon` name and the `mimetypes` copies serve two different lookups, and the install writes both.

### Why xdg-mime Is Required

The project writes its definition, its entry, and its icons itself, and only the default association is written by another program.
`xdg-mime` implements the association format, and hand-writing `mimeapps.list` would risk a file that GIO does not read as intended.
Install therefore refuses when `xdg-mime` is missing, with the message `xdg-mime is required to register a MIME default; install xdg-utils and retry`, and it refuses before writing anything.
The other helper programs are optional: `update-mime-database`, `update-desktop-database`, and the GTK icon-cache tool are each invoked only when they exist, and their absence degrades the result instead of failing the install.

## The Install Command Sequence

An install runs its helper commands in this order:

- `update-mime-database $XDG_DATA_HOME/mime`, after the definition is written.
- `gtk4-update-icon-cache -f -t $XDG_DATA_HOME/icons/hicolor`, or `gtk-update-icon-cache -f -t`, after the icons are written.
- `xdg-mime default pkpass-handler.desktop application/vnd.apple.pkpass application/vnd.apple.pkpasses`, one invocation covering both types.
- `update-desktop-database $XDG_DATA_HOME/applications`, after the entry is written.

The order follows the dependencies.
`update-mime-database` must see the definition before the compiled cache can describe the types.
`xdg-mime default` must see the entry, and `update-desktop-database` must see it too so that the entry appears in `mimeinfo.cache`.
The definition, the entry, and the icons are written first, and the record is written last.

## Inspecting the Result

These commands show what the desktop believes, in order of increasing specificity:

- `xdg-mime query default application/vnd.apple.pkpass`, which should print `pkpass-handler.desktop`.
- `gio mime application/vnd.apple.pkpass`, which lists the default and the registered applications for the type as GIO resolves them.
- `gio info -a standard::content-type FILE.pkpass`, which should print `application/vnd.apple.pkpass` for a file named with the extension.
- `file --mime-type FILE.pkpass`, which prints `application/zip`, because libmagic has no pass rule; a ZIP answer here is not a fault.
- `xdg-mime query filetype FILE.pkpass`, which asks shared-mime-info through the `xdg-mime` interface.
- `pkpass-integrate handler status`, which reports the default for each type, the entry path, whether the definition is present and installed by this project, whether each type icon is present, and whether the record is present.

The type is a hint from the file name and the first bytes, and a `.pkpass` file that fails every content check still types as a pass.
Validation belongs to the reader, which opens the archive and checks the members rather than trusting the MIME type.

## Sources

- freedesktop.org specifications: [Shared MIME-info Database](https://specifications.freedesktop.org/shared-mime-info-spec/latest/), [associations between applications and MIME types](https://specifications.freedesktop.org/mime-apps-spec/latest/), [desktop entry](https://specifications.freedesktop.org/desktop-entry-spec/latest/), and [base directories](https://specifications.freedesktop.org/basedir-spec/latest/).
- Manual pages: [update-mime-database(1)](https://manpages.debian.org/bookworm/shared-mime-info/update-mime-database.1.en.html), [update-desktop-database(1)](https://manpages.debian.org/bookworm/desktop-file-utils/update-desktop-database.1.en.html), [xdg-mime(1)](https://manpages.debian.org/bookworm/xdg-utils/xdg-mime.1.en.html), and [gio(1)](https://manpages.debian.org/bookworm/libglib2.0-bin/gio.1.en.html).
- shared-mime-info sources: [freedesktop.org.xml.in](https://gitlab.freedesktop.org/xdg/shared-mime-info/-/raw/master/data/freedesktop.org.xml.in), the upstream type definitions, and [NEWS](https://gitlab.freedesktop.org/xdg/shared-mime-info/-/raw/master/NEWS), which records the release that added each type.
- [IANA media types registry](https://www.iana.org/assignments/media-types/media-types.xhtml), which has no pass entry.
- This project, the definition XML, the desktop entry text, and the install sequence: [handler_integrator.cpp](../sources/integration/handler_integrator.cpp) and [handler_integrator.h](../sources/integration/handler_integrator.h), which hold the types, names, and paths.
- This project, the `handler` and `handle` commands with the status report: [pkpass_integrate.cpp](../sources/tools/pkpass_integrate.cpp).
- This project, the icon installed under three names: [pkpass-handler.svg](../sources/tools/icons/pkpass-handler.svg).
