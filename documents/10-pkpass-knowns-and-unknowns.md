# The `.pkpass` Format: What Is Known, and What Is Not

This document separates what is **documented**, what has been **observed**,
what is only **convention**, and what is genuinely **unknown** about the format
this project reads.
It is deliberately about the state of knowledge rather than about the format
itself: `documents/08-pkpass-format.md` describes the format, field by field,
and this document says how far each of those statements can be trusted.
Facts were checked on 2026-10-10.

## How to Read This

Every claim below carries one of these labels.

| Label | Meaning |
| --- | --- |
| **Documented** | Stated by Apple, in its current Wallet Passes documentation or in its archived PassKit Package Format Reference. |
| **Observed** | Measured here, on this machine or against real passes, and reproducible from this repository. |
| **Convention** | Not documented by Apple, but consistent across implementations or required by the freedesktop stack. |
| **Choice** | The format is silent and this project decided; `tests/` pins the decision. |
| **Unknown** | Not stated in any authoritative source, or not verifiable in this environment. |

A label attached to a heading applies to everything under it.

## 1. The Standards Question

- There is no standard for `.pkpass`. **Unknown / None** — no ISO/IEC, no W3C
  Recommendation, no IETF RFC, and no ECMA or Open Group specification exists
  for the format, and none is in preparation as far as anything public shows.
- `application/vnd.apple.pkpass` is **not registered with IANA**. **Observed** —
  the type's entry in the IANA media-types registry returns HTTP 404, and the
  string `pkpass` does not appear in the registry at all.
  The `vnd.apple.*` types that *are* registered are `vnd.apple.installer`,
  `vnd.apple.keynote`, `vnd.apple.mpegurl`, `vnd.apple.numbers`,
  `vnd.apple.pages`, and `vnd.apple.steering-list`.
- Apple publishes documentation, not a specification. **Documented** — the
  current reference is written as a JSON API reference for `pass.json`, and it
  does not describe the container at all; the container was described by the
  archived *PassKit Package Format Reference*, which Apple withdrew from its
  live documentation and which now redirects to a JavaScript application.
- The documentation is machine-readable, which is the nearest thing to a
  schema. **Observed** — Apple serves each documentation page as Markdown (append
  `.md`) and the whole framework as one JSON document at
  `developer.apple.com/tutorials/data/documentation/walletpasses.json`.
  Nothing states that either is stable or versioned.
- There is no conformance suite and no validator. **Unknown** — nothing
  authoritative states what "a valid pass" is beyond "Wallet accepts it", and
  Wallet's acceptance rules are not published.
- The de facto specifications are the open-source implementations.
  **Convention** — KDE's KPkPass, GNOME's Passes, the Rust `pkpass` and `passes`
  crates, and php-pkpass are widely used; where they agree, that agreement is
  the closest thing to an interoperable definition.

## 2. What Is Documented

**Documented.** This section is an inventory of what Apple states, with pointers
into `documents/08-pkpass-format.md` for the detail; it does not repeat the
field tables.

### 2.1 The Container

- A `.pkpass` is a ZIP whose extension was changed. Apple states the procedure:
  create a PKCS#7 detached signature for the manifest, zip the directory, and
  rename the archive.
- `pass.json`, `manifest.json`, and `signature` are required members.
- `manifest.json` maps every other member's path to its SHA-1 digest, in
  lowercase hexadecimal, and excludes itself and `signature`.
- `signature` is a detached PKCS#7 (CMS) structure over the bytes of
  `manifest.json`.
- An icon is required in practice; Apple's archived guide states that all passes
  require one.
- A bundle of passes is a ZIP of `.pkpass` files renamed `.pkpasses`, with the
  MIME type `application/vnd.apple.pkpasses`, at most ten passes or 150 MB.

### 2.2 `pass.json`

- `formatVersion` is `1`.
- The required keys are `passTypeIdentifier`, `serialNumber`, `teamIdentifier`,
  `organizationName`, and `description`, alongside `formatVersion`.
- Exactly one style key is present: `generic`, `storeCard`, `coupon`,
  `eventTicket`, or `boardingPass`.
- The field groups are `headerFields`, `primaryFields`, `secondaryFields`,
  `auxiliaryFields`, and `backFields`, plus `additionalInfoFields` for poster
  event tickets.
- A field carries `key` and `value`, and may carry `label`, `attributedValue`,
  `changeMessage`, `textAlignment`, `dateStyle`, `timeStyle`, `currencyCode`,
  `numberStyle`, `dataDetectorTypes`, `semantics`, and `row`.
- `boardingPass` requires `transitType`; the other styles must not carry it.
- The colours are `backgroundColor`, `foregroundColor`, and `labelColor`, given
  as `rgb(r,g,b)`, plus `footerBackgroundColor` for poster event tickets.
- Barcodes appear as a `barcodes` array, and historically as a singular
  `barcode` object; each carries `format`, `message`, `messageEncoding`, and
  optionally `altText`.
- `webServiceURL` requires `authenticationToken`, must use HTTPS, and the token
  is at least sixteen characters.
- `locations`, `beacons`, and `nfc` describe relevance and NFC; an NFC message
  is at most 64 bytes.
- Localization uses `<language>[-<region>].lproj/` directories containing
  `pass.strings`, and a `pass.json` value that names a key in that catalog.
- Images are `icon`, `logo`, `strip`, `thumbnail`, `background`, and `footer`,
  each with `@2x` and `@3x` variants.

### 2.3 What Apple Does Not Document, but Describes Informally

- Field counts and per-style image availability are given as guidance rather
  than as constraints, in prose.
- `attributedValue` is described as supporting an HTML anchor, and as
  overriding `value`.
- The pass structure grows over time; Apple adds keys without stating a
  versioning policy.

## 3. What Has Been Observed Directly

**Observed.** Each of these was measured, and each is reproducible from this
repository except where noted.

- A real pass mixes compression methods within one archive: PNG members stored,
  JSON and logo members deflated. This is why the reader honours each member's
  own method rather than assuming one.
- `manifest.json` may be the *first* member, so the magic rule the freedesktop
  database uses (`PK\003\004` at offset 0 followed by `pass.json` at offset 30)
  does not match a real pass. Detection then rests on the `*.pkpass` glob alone.
- Consequently any ZIP named `*.pkpass` is typed as a pass, including one that
  contains no pass at all: **a reader must validate the contents and must not
  trust the MIME type**.
- `file`/libmagic reports `application/zip` for a `.pkpass`; GIO and `xdg-mime`
  report `application/vnd.apple.pkpass`. Only the latter is useful.
- A real pass may use the legacy singular `barcode` object rather than the
  array, and may declare `messageEncoding` as `utf-8` rather than Apple's
  documented example of `iso-8859-1`.
- A barcode `message` may itself be a JSON document encoded as a string.
- The manifest's digests are SHA-1, while the CMS structure's own digest
  algorithm is SHA-256. The two are independent.
- A real signature embeds the signing certificate and Apple's WWDR intermediate,
  but not Apple's root, so a chain verification fails offline while the content
  verification succeeds with `-noverify`.
- shared-mime-info has shipped `application/vnd.apple.pkpass` since version 2.2
  and `application/vnd.apple.pkpasses` since 2.5. A distribution older than 2.2
  has no definition at all, so the project installs its own.
- The archived PassKit Package Format Reference is no longer reachable at its
  live URL; the text is readable through the Wayback Machine.

## 4. What Is Convention Rather Than Documented

**Convention.**

- The MIME type string `application/vnd.apple.pkpass` for a single pass. Apple
  documents a MIME type only for the `.pkpasses` bundle.
- The Unix type identifier `com.apple.pkpass`. It is corroborated by
  Apple-ecosystem sources but does not appear in a published Apple table.
- Hexadecimal colour forms. Apple documents `rgb(r,g,b)` only; every
  open-source reader also accepts hex, and the project does too.
- `stripColor` is folklore: it appears in no Apple documentation and should not
  be implemented as a pass key.
- The exact glob and magic a Linux system should register for the type.
- That a scanner ignores pad codewords and any trailing bytes in a barcode.

## 5. What Is Not Known

**Unknown.** Each entry says why it is unknown and why it matters.

### 5.1 The Container

- **Whether members may live in subdirectories.** Apple's examples are flat, and
  its prose says a file's path appears in the manifest, which implies nesting is
  representable, but nothing states whether Wallet accepts it.
  This project requires `pass.json` at the archive root.
- **The archive's permitted feature set.** "Zip it" does not say whether ZIP64,
  data descriptors, archive comments, or encryption are acceptable.
  Apple's own builds appear to use the plain form.
- **Filename encoding.** ZIP permits CP437 and UTF-8, and the manifest keys are
  paths. Which encoding a pass uses, and how the manifest matches it, is not
  stated.
- **Whether the manifest must be exhaustive.** Apple says every file appears in
  the manifest, but it is not stated what Wallet does with an unlisted member or
  a listed member that is absent.
- **What a digest covers.** That it is the member's raw bytes is stated for the
  manifest; how a filename is encoded before hashing is not.

### 5.2 Signature and Trust

- **Which certificate chain Wallet requires.** The signing certificate is issued
  by Apple's Pass Type ID CA and chains through Apple's WWDR intermediate to
  Apple's root, but the acceptance rules, the required root, and whether a
  short chain is tolerated are not published.
- **Whether the CMS digest algorithm is constrained.** SHA-256 was observed on
  one pass; whether other algorithms are accepted is unknown.
- **Whether revocation is checked, and when.** Nothing public describes Wallet's
  CRL or OCSP behaviour.
- **Whether an invalid signature stops a pass being added.** The signature is
  required, but Apple does not state the enforcement, so a viewer cannot know
  what Wallet would do with the same file.
- **What happens to an expired signing certificate.** Undocumented.

### 5.3 Validation and Rendering

- **What happens to an unknown key.** Apple adds keys over time and states no
  policy, so it is unknown whether unknown keys are ignored, rejected, or
  preserved.
- **What happens to an invalid value.** An unreadable colour, an out-of-range
  number, or a malformed date has no stated behaviour.
- **The exact rule for choosing among several barcodes.** Apple says the system
  uses "the first displayable barcode", which leaves open what counts as
  displayable and what a non-Apple renderer should do.
  This project draws the first barcode it can encode.
- **How `dateStyle`, `timeStyle`, `currencyCode`, and `numberStyle` render.**
  They map to platform formatter styles, whose output depends on locale and
  platform version. A Linux renderer can only show the value as written.
- **Whether image sizes and aspect ratios are enforced.** Point sizes appear in
  the archived guide, and nothing states what happens to a pass that ignores
  them.
- **Whether `strip`, `background`, and `thumbnail` are mutually exclusive.** The
  archived guide advises against combining some of them for a style; whether
  that is enforced is unknown.
- **How `voided`, `expirationDate`, and `relevantDate` affect display.** The
  lock-screen behaviour is platform policy and is not specified.
- **`pass.strings` encoding.** The archived guide says to use UTF-16 for
  non-ASCII characters, while passes in the wild carry UTF-8. Nothing states
  what a reader must accept.
- **Whether `.pkpasses` is a plain ZIP of passes with the same member rules.**
  Apple states the container and its limits, not the structure's details.

### 5.4 The Newer, Moving Parts

These are the areas where the documentation is newest or thinnest, and where a
statement of fact has the shortest half-life.

- Poster event tickets, and the keys that came with them: `additionalInfoFields`,
  `footerBackgroundColor`, `useAutomaticColors`, `suppressHeaderDarkening`, and
  the family of venue and merchandise URLs.
- Semantic tags (`semantics`) and their allowed vocabulary.
- `personalization.json` and the NFC rewards enrollment flow.
- The newer barcode formats — Code 39, Codabar, EAN-13, and Interleaved 2 of 5 —
  which appear in the current documentation and in KDE's table but not in the
  archived reference.
- `upcomingPassInformation`, `relevantDates`, and the newer airline and transit
  URL families.

## 6. What Is Not Knowable From Here

**Unknown**, and not merely undocumented: these cannot be settled without
hardware and software this project does not have.

- **Whether Wallet accepts a pass.** No macOS or iOS device is available, so no
  pass this project reads, renders, or writes a symbol for has been offered to
  Wallet for acceptance.
- **Whether a rendered card looks like what Wallet shows.** The card is this
  project's reading of the documented layout, not a reproduction of Apple's.
- **Whether Apple's own signing tool would accept a pass.** `signpass` and the
  Pass Type ID tooling require a macOS host and an Apple developer account.
- **Whether the chain verification of a real signature succeeds.** It cannot be
  completed offline because Apple's root is not in the pass and is not in this
  machine's trust store.
- **Whether a crafted pass is rejected.** Apple publishes no test corpus, so
  there is no reference for "known bad".

## 7. Choices This Project Made Where the Format Is Silent

**Choice.** These are not facts about the format; they are this project's
decisions, and `tests/` holds them in place.

- Four- and eight-digit hex colours are read as alpha-leading (`#AARRGGBB`),
  following Qt and KDE's reader, rather than CSS's `#RRGGBBAA`.
- A colour that cannot be read becomes a warning and the pass keeps its default
  colours; it never fails the pass.
- The symbol encodes the message's UTF-8 bytes, whatever `messageEncoding` says.
  Transcoding to the declared character set is **not** implemented.
- The first barcode is used, whatever its format; if it is not a QR, the card
  says the symbol could not be encoded.
- Only `http` and `https` become links, and only `attributedValue` is treated as
  markup.
- The reader prefers the `barcodes` array over the legacy singular `barcode`, and
  never reports both as the same entry.
- A structurally readable pass is returned with warnings rather than refused.

## 8. What This Project Does Not Implement

**Unknown** from the reader's point of view: these parts of the format are not
read at all, so a pass that uses them is shown incompletely.

- `<language>.lproj/pass.strings` catalogs, and the key indirection they imply.
- `.pkpasses` bundles: the type is registered, but a bundle is not unpacked.
- `additionalInfoFields`, `semantics`, `beacons`, `maxDistance`,
  `personalization.json`, `upcomingPassInformation`, and the poster and airline
  extras.
- Barcode formats other than QR: PDF417, Aztec, Code 128, Code 39, Codabar,
  EAN-13, and Interleaved 2 of 5 are reported but not drawn.
- Character-set conversion for `messageEncoding`.
- Certificate chain validation.
- Any judgement about whether a pass is acceptable to Wallet.

## 9. How These Unknowns Could Be Reduced

- **Snapshot the machine-readable documentation and diff it.** Reading
  `walletpasses.json` on a date and comparing with a later copy turns "the docs
  may have moved" into a concrete, dated finding. This is the cheapest available
  improvement.
- **Follow the implementations, not only Apple.** shared-mime-info, KPkPass, and
  GNOME's Passes encode de facto behaviour for the grey areas, and a
  disagreement between two of them is a reliable signal that the area is
  undocumented.
- **Collect passes and test invariants empirically.** Manifests, compression
  mixes, and `messageEncoding` values in the wild are measurable; running the
  reader over a corpus would replace several "may" statements above with
  observations.
- **Probe with a device.** Adding a pass to Wallet on real hardware is the only
  way to settle enforcement, rendering, and chain-trust questions.
- **Ask the vendor.** Apple's developer forums and Wallet release notes are the
  only channel for a ruling on the undocumented points.
- **Keep the unknowns visible.** A document like this one is only useful while
  it is dated and reviewed; an un-dated claim about a moving format becomes a
  false statement that nobody notices.

## Sources

The format reference keeps the full list of sources. The ones this document
leans on:

- Wallet Passes, current documentation — <https://developer.apple.com/documentation/walletpasses>
- Machine-readable form of the same — <https://developer.apple.com/tutorials/data/documentation/walletpasses.json>
- PassKit Package Format Reference, archived (package structure, top-level keys,
  lower-level keys, field dictionary) — <https://developer.apple.com/library/archive/documentation/UserExperience/Reference/PassKit_Bundle/Chapters/LowerLevel.html>
- Distributing and updating a pass (the `.pkpasses` bundle and its MIME type) —
  <https://developer.apple.com/documentation/walletpasses/distributing-and-updating-a-pass>
- IANA media types registry, which has no `pkpass` entry —
  <https://www.iana.org/assignments/media-types/media-types.xhtml>
- shared-mime-info, the freedesktop type database and its definition of the
  type — <https://gitlab.freedesktop.org/xdg/shared-mime-info/-/raw/master/data/freedesktop.org.xml.in>
- PKPass, Just Solve the File Format Problem —
  <http://fileformats.archiveteam.org/index.php?title=PKPass>
- KPkPass, KDE's reader — <https://invent.kde.org/pim/kpkpass>
- Passes, the GNOME reader — <https://github.com/pablo-s/passes>
