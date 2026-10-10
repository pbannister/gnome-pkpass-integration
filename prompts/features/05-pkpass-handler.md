# Feature: PKPASS Handler

## Purpose

Double-clicking a `.pkpass` must open it, and `*.pkpass` files must carry this
project's icon. That means registering the two Wallet MIME types with the
desktop, pointing them at this project's handler, and being able to reverse
every change.

## Requirements

- `PKPASS-HANDLER-R001` — The handler must define `application/vnd.apple.pkpass` for `*.pkpass` and `application/vnd.apple.pkpasses` for `*.pkpasses`, including a magic test and this project's generic icon.
- `PKPASS-HANDLER-R002` — The handler must write the definition to `$XDG_DATA_HOME/mime/packages/pkpass.xml` and refresh the MIME database with `update-mime-database` when that tool is present.
- `PKPASS-HANDLER-R003` — The handler must back up a definition at that path that the project did not write, and must restore it byte for byte on uninstall.
- `PKPASS-HANDLER-R004` — The handler must remove a definition it created on uninstall, and must record which of the two cases applies.
- `PKPASS-HANDLER-R005` — The handler must write `$XDG_DATA_HOME/applications/pkpass-handler.desktop` with `Type`, `Name`, `Exec`, `Icon`, `StartupWMClass`, `Terminal`, `NoDisplay`, `MimeType` for both types, and the `X-Integrated-By` and `X-Integrated-At` keys.
- `PKPASS-HANDLER-R006` — The handler entry's `Exec` must be `<tool> handle %f`, with the path quoted when it contains a space or a reserved character.
- `PKPASS-HANDLER-R007` — The handler must install its icon as `hicolor/scalable/apps/pkpass-handler.svg` and a file-type icon for each MIME type as `hicolor/scalable/mimetypes/<type>.svg` under `$XDG_DATA_HOME/icons`.
- `PKPASS-HANDLER-R008` — The handler must refresh the GTK icon cache after writing an icon, using `gtk4-update-icon-cache` or `gtk-update-icon-cache` when one is present.
- `PKPASS-HANDLER-R009` — The handler must make `pkpass-handler.desktop` the default for both types with `xdg-mime`, and must refuse to install when `xdg-mime` is absent.
- `PKPASS-HANDLER-R010` — The handler must refresh the desktop database with `update-desktop-database` when that tool is present.
- `PKPASS-HANDLER-R011` — The handler must record the previous default for each type before replacing it, and must reuse an already recorded value when reinstalling over itself.
- `PKPASS-HANDLER-R012` — The handler must restore each recorded previous default on uninstall, and must leave a type alone when no previous default was recorded.
- `PKPASS-HANDLER-R013` — The handler must record handler_desktop, icon, mime_package, mime_backup, mime_created, and one previous_default per type in `$XDG_DATA_HOME/gnome-pkpass-integration/pkpass-handler.manifest`, as `key=value` lines.
- `PKPASS-HANDLER-R014` — Installing twice must be harmless: one handler, one record, and the project must keep ownership of a definition it created.
- `PKPASS-HANDLER-R015` — `handler status` must report the default recorded for each type, the entry and its presence, whether the definition is present and whether the project owns it, the two type icons, and the record.
- `PKPASS-HANDLER-R016` — `handler uninstall` must fail with a message when there is no record to undo.
- `PKPASS-HANDLER-R017` — `handle FILE` must run the graphical viewer when one is installed and a display is available, and must otherwise print the equivalent command-line commands and exit zero.
- `PKPASS-HANDLER-R018` — `handle FILE` must fail when the file does not exist.
- `PKPASS-HANDLER-R019` — Nothing in the integration may require root, and no path outside `$XDG_DATA_HOME`, `$XDG_CONFIG_HOME`, and the temporary directory may be written.
- `PKPASS-HANDLER-R020` — No public interface may throw.

## Behavior

- Given a fresh user directory, `handler install` writes the definition, the entry, the three icons, and the record, and both types resolve to `pkpass-handler.desktop`.
- Given a definition the project did not write, install backs it up and uninstall restores it unchanged.
- Given a definition the project created, uninstall removes it.
- Given no record, `handler uninstall` exits nonzero.
- Given no display and no viewer, `handle` prints the `pkpass-inspect` and `pkpass-render` commands and exits zero.

## Dependencies

- `03-pkpass-reader.md`
