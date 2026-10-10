# The Apple Wallet `.pkpass` Format

A `.pkpass` file is an Apple Wallet pass: a ZIP archive whose `pass.json` describes a pass, whose `manifest.json` records a SHA-1 digest per member, and whose `signature` carries a detached PKCS#7 signature over the manifest.
This document is the format reference for the readers in this project, and its findings were checked on 2026-10-10 against the sources at the end.
Every example uses the repository's synthetic fixture in `dataflow.in/pkpass/sample/` or an invented neutral value; no value from a real person's pass appears here.

## The Container

### A `.pkpass` Is a ZIP

Apple's *Building a Pass* says to create a PKCS #7 detached signature for the manifest, zip the resulting directory, and change the archive's extension from `.zip` to `.pkpass`; the archived *Package Structure* chapter states the same.

- The extension is `.pkpass`.
- A bundle of passes is a ZIP of `.pkpass` files renamed `.pkpasses`, with the MIME type `application/vnd.apple.pkpasses`; Apple limits a bundle to ten passes or 150 MB.
- Apple does not document a MIME type for a single pass and no `pkpass` type is registered with IANA; the conventional string is `application/vnd.apple.pkpass`, which shared-mime-info has shipped since version 2.2.
- The type is unregistered, so a reader must validate the archive's contents and never trust the MIME type or the extension alone.

### Required Members

| Member | Status | Purpose |
|---|---|---|
| `pass.json` | required | UTF-8 JSON object that defines the pass |
| `manifest.json` | required | maps each member path to a SHA-1 digest |
| `signature` | required | detached PKCS#7/CMS signature over `manifest.json` |
| `icon.png` | required in practice | Apple writes "All passes require an icon"; used in notifications, Mail attachments, and the lock screen |

An archive missing `pass.json` is not a pass at all, whatever else it contains.

### Optional Image Members

Apple loads pass images with the standard `UIImage` rules, so each family accepts an unsuffixed file plus `@2x` and `@3x` variants.

| Family | Typical role |
|---|---|
| `logo` | drawn beside `logoText` in the header |
| `strip` | a banner behind the primary fields |
| `thumbnail` | a small image to the left of the primary fields |
| `background` | a full-bleed image behind the pass |
| `footer` | a narrow image at the bottom of a boarding pass |

The optional families are exactly `logo`, `strip`, `thumbnail`, `background`, and `footer`; the icon is not optional, and Apple's *Pass Design and Creation* restricts which styles may carry which families, as the style table below records.

### Localization Directories and Personalization

A pass may carry one directory per language at its root, named `<language>[-<region>].lproj/`, each holding a `pass.strings` catalog and possibly localized copies of the images.
Each localization folder must contain the same number of resolutions for an image, and an image of the same name in the top-level folder overrides every localized version.
Apple's *Rewards Enrollment* documentation adds an optional `personalization.json` for NFC reward cards, with `personalizationLogo@2x.png` and `personalizationLogo@3x.png`; a reader that does not implement enrollment can list the member and ignore it.

### Compression

ZIP entries may be stored (method 0) or deflated (method 8), and both occur in real passes.
A reader must honour each entry's own compression method and must not assume that the first entry is `pass.json`.

## `manifest.json`

`manifest.json` is a JSON object whose keys are member paths relative to the pass root and whose values are the SHA-1 digests of those members' raw bytes, written as lowercase hexadecimal.
Apple's archived *Package Structure* says every file in the bundle appears in the manifest except the manifest itself and the signature.

The fixture's manifest is short enough to show whole:

```json
{
    "pass.json": "9d8e6778ee0d0d141e96641012225aa43e04b3f4",
    "icon.png": "2b7e4e13fe9603e28bb5ee5dbc31db8c882a98c3",
    "logo.png": "3c2bb1b8bac5e7f48f812e54cd711b344053ba84"
}
```

- The three digests above are the true SHA-1 values of the three fixture files.
- `manifest.json` and `signature` are absent from their own manifest.
- A localized member appears as a path such as `en.lproj/logo.png`, with the slash escaped as `\/` in the JSON text.
- A member present in the archive but absent from the manifest is a separate fault from a digest that does not match; a mail gateway that re-encodes an image is a common cause.

## The Signature

`signature` is a detached PKCS#7/CMS `SignedData` structure, DER-encoded, whose signed content is the raw byte sequence of `manifest.json`.

- The CMS digest is not the manifest's SHA-1: observed passes sign with SHA-256 even though the manifest digests are SHA-1.
- The signing certificate is an Apple-issued Pass Type ID certificate, and its subject names the pass type and the team.
- Apple embeds the WWDR intermediate but not the Apple Root CA, so an offline Linux host usually cannot build the chain.
- The content check and the chain check are therefore separate findings: "present but not trusted" is not the same as "invalid".
- The content check needs no Apple trust material, because `-noverify` skips chain building:

```bash
openssl smime -verify -inform DER -in signature -content manifest.json -binary -noverify
```

- Dropping `-noverify` and supplying the Apple WWDR plus Root CA in the trust store performs the chain check as well.
- The signer can be inspected without any trust material with `openssl pkcs7 -inform DER -in signature -print_certs -noout`.

## `pass.json`

`pass.json` is a single UTF-8 JSON object; Apple's current documentation calls it the `Pass` object, and the archived reference calls the top level a dictionary.

### Required Top-Level Keys

| Key | Type | Notes |
|---|---|---|
| `formatVersion` | number | must be `1` |
| `passTypeIdentifier` | string | must equal the pass type ID in the signing certificate |
| `serialNumber` | string | unique within the pass type identifier |
| `teamIdentifier` | string | the Apple Developer team ID, and it must match the certificate |
| `organizationName` | localizable string | display name of the issuing organization |
| `description` | localizable string | accessibility description, read by VoiceOver |

Apple's *Common Problems* checklist confirms the certificate pairings, and it treats `organizationName` and `description` as required, so both appear above rather than among the optional keys.

### Optional Top-Level Keys

| Key | Type | Notes |
|---|---|---|
| `logoText` | localizable string | text beside the logo; not used by poster event tickets |
| `relevantDate` | ISO 8601 date string | deprecated in favour of `relevantDates` |
| `expirationDate` | ISO 8601 date string | must include hours and minutes, seconds optional |
| `voided` | boolean | default `false`; marks the pass void |
| `groupingIdentifier` | string | event tickets and boarding passes only; groups passes of one style and type |
| `sharingProhibited` | boolean | default `false`; `true` hides the Share button |
| `suppressStripShine` | boolean | suppresses the strip's shine; current documentation gives the default as `true` |
| `maxDistance` | number | relevance radius in metres; the system uses the smaller of this and its own default |
| `associatedStoreIdentifiers` | array of numbers | App Store IDs; Wallet uses the first compatible one |
| `appLaunchURL` | string | requires `associatedStoreIdentifiers`; not honoured on watchOS |
| `userInfo` | any JSON | opaque to Wallet and intended for a companion app |
| `webServiceURL` | string | must be HTTPS |
| `authenticationToken` | string | required with `webServiceURL`, and at least 16 characters |
| `nfc` | object | a `pass.json` key and not an archive member; `message` of at most 64 bytes, an `encryptionPublicKey`, and an optional `requiresAuthentication` |
| `semantics` | object | machine-readable tags for the pass and its fields |
| `locations` | array of objects | relevance locations, at most ten |
| `beacons` | array of objects | iBeacon relevance: `proximityUUID` required, `major` and `minor` are 16-bit unsigned integers |

- `webServiceURL` and `authenticationToken` travel together: the token is required when the URL is present and is not allowed on its own.
- `relevantDates` is a newer array that supersedes `relevantDate`, with `date`, `startDate`, and `endDate` entries; `endDate` requires `startDate`.
- Poster event tickets and airline boarding passes add further keys of their own, and Apple keeps adding more, so a reader should preserve and display unknown keys rather than reject the pass.

## Pass Styles

A pass provides exactly one style key, and that key's value is the pass structure dictionary holding the fields; the five styles are `generic`, `storeCard`, `coupon`, `eventTicket`, and `boardingPass`.

| Style | Front layout | Images allowed |
|---|---|---|
| `generic` | primary fields prominent | logo, icon, thumbnail |
| `storeCard` | primary fields prominent, with the strip behind them | logo, icon, strip |
| `coupon` | primary fields prominent, with the strip behind them | logo, icon, strip |
| `eventTicket` | primary fields render in the header row; secondary and auxiliary fields below | logo, icon, strip, background, thumbnail |
| `boardingPass` | header, then an origin/destination pair of primary fields, then secondary and auxiliary fields | logo, icon, footer |

- For an event ticket, supply a strip or a background or a thumbnail, not all three; a strip excludes the other two.
- A pass with no style key, or with more than one, is malformed; the fixture uses `generic` with an icon and a logo only.

## The Field Model

The style dictionary holds up to six groups of field objects.

| Group | Role |
|---|---|
| `headerFields` | a small row of labels above the primary fields |
| `primaryFields` | the one field the pass is about |
| `secondaryFields` | supporting values below the primary field |
| `auxiliaryFields` | further values, usually on one or two rows |
| `backFields` | the reverse of the pass |
| `additionalInfoFields` | an "Additional Info" section on poster event tickets only |

- Apple's archived *Pass Design and Creation* gives the guidance of at most three header fields, one primary field, four secondary fields, and four auxiliary fields.
- A boarding pass is the exception, allowing up to two primary fields and up to five auxiliary fields.
- Coupons, store cards, and generic passes with a square barcode should keep the combined secondary and auxiliary count to four.
- Long values silently push later fields off the pass, so extra fields are not guaranteed to display.

### Per-Field Keys

| Key | Type | Notes |
|---|---|---|
| `key` | string | required, and unique across the whole pass |
| `value` | localizable string, date string, or number | required; a date value must carry a time zone |
| `label` | localizable string | the caption drawn above or beside the value |
| `attributedValue` | localizable string, date, or number | overrides `value`; only an HTML `<a href="…">` link is supported |
| `changeMessage` | localizable format string | must contain `%@`, replaced by the new value; it drives update notifications |
| `textAlignment` | string | `PKTextAlignmentLeft`, `Center`, `Right`, or `Natural`; not valid on primary or back fields |
| `dateStyle` | string | `PKDateStyleNone`, `Short`, `Medium`, `Long`, or `Full` |
| `timeStyle` | string | the same enumeration as `dateStyle` |
| `currencyCode` | string | ISO 4217 code; only when the value is a number |
| `numberStyle` | string | `PKNumberStyleDecimal`, `Percent`, `Scientific`, or `SpellOut`; only for numeric values |
| `dataDetectorTypes` | array of strings | back fields only; `PKDataDetectorTypePhoneNumber`, `Link`, `Address`, or `CalendarEvent` |
| `semantics` | object | semantic tags for this field |
| `row` | number | event-ticket auxiliary fields only |

- Give `dateStyle` and `timeStyle` together or give neither, and when either is present the value is treated as a date.
- `currencyCode` and `numberStyle` are mutually exclusive within one field, and the style names follow Cocoa's `NSDateFormatter` and `NSNumberFormatter` styles.
- `iso8601` is not an Apple field key; an ISO 8601 date is expressed by the value plus `dateStyle` and `timeStyle`.
- `row` exists only on an event ticket's auxiliary fields, where `"row": 1` opens a second row of up to four fields.
- A reader must render both the plain string form and the pre-formatted date strings that many real passes use:

```json
"generic": {
    "primaryFields": [ { "key": "holder", "value": "Ada Lovelace" } ],
    "secondaryFields": [ { "key": "event_name", "label": "EVENT", "value": "Example Event" } ],
    "backFields": [ { "key": "description", "label": "How to Use Your Barcode", "value": "Show your barcode to the event staff." } ]
}
```

## Colours

Apple documents four colour keys, all optional strings, all described as a CSS-style RGB triple.

| Key | Role |
|---|---|
| `backgroundColor` | the pass background |
| `foregroundColor` | the value text |
| `labelColor` | the label text; when omitted the system chooses one |
| `footerBackgroundColor` | the footer band of a poster event ticket |

- `stripColor` does not exist: it appears nowhere in Apple's current Wallet Passes documentation, the archived *PassKit Package Format Reference*, or the archived *Wallet Developer Guide*.
- The documented form is `rgb(r,g,b)` with decimal integers from 0 to 255; spaces after the commas are allowed and common, so `rgb(86, 39, 126)` is valid and a pattern that forbids whitespace fails on real passes.
- `rgba(r,g,b,a)` is not in Apple's documentation but is accepted by open-source readers.
- No hexadecimal form is documented by Apple, yet KDE's KPkPass falls back to `QColor(string)`.
- The GNOME Passes release notes also mention fixing hex colour parsing, so an accepting reader handles `#RGB`, `#RRGGBB`, and `#AARRGGBB` with the alpha channel first in the four-channel form.
- An unreadable colour should become a warning and a default colour, never a failed pass.
- Rendering order is the background image or colour, then the strip, then the logo and thumbnail, then the field text.

## Barcodes

A pass may carry the legacy singular `barcode` object, the `barcodes` array, or both.

```json
"barcode": {
    "format": "PKBarcodeFormatQR",
    "message": "example-payload-0001",
    "messageEncoding": "utf-8",
    "altText": "ID: 0000000001"
}
```

| Key | Required | Notes |
|---|---|---|
| `format` | yes | one of the format constants below |
| `message` | yes | the payload; it may itself be a JSON string |
| `messageEncoding` | yes | an IANA character set name such as `iso-8859-1` or `utf-8` |
| `altText` | no | human-readable text drawn near the barcode; not shown on watchOS |

- Encode the message bytes using `messageEncoding` before drawing a symbol, and never assume ASCII.
- The classic format constants are `PKBarcodeFormatQR`, `PKBarcodeFormatPDF417`, `PKBarcodeFormatAztec`, and `PKBarcodeFormatCode128`.
- The legacy singular `barcode` object accepted only QR, PDF417, and Aztec; Code 128 arrived with the array in iOS 9.
- Apple's current documentation widens the set to QR, PDF417, Aztec, Code 128, Code 39, Codabar, EAN-13, and Interleaved 2 of 5, whose constants are `PKBarcodeFormatCode39`, `PKBarcodeFormatCodabar`, `PKBarcodeFormatEAN13`, and `PKBarcodeFormatI2of5`.
- Wallet uses the first displayable entry of the `barcodes` array, so a reader takes `barcodes[0]` when the array is non-empty, otherwise the singular `barcode`, otherwise nothing.
- A reader that supports only some formats should still show `altText` and the message as text, and say that the format is unsupported.
- Apple's historical advice was to emit both shapes at once so that old and new iOS releases both work.

## Boarding Passes and Event Tickets

A `boardingPass` dictionary carries `transitType`, which Apple documents as required there and not allowed elsewhere; its values are `PKTransitTypeAir`, `PKTransitTypeBoat`, `PKTransitTypeBus`, `PKTransitTypeGeneric`, and `PKTransitTypeTrain`.
Wallet uses the value for iconography, and a reader may map each to its own icon while rejecting the key on other styles.
An `eventTicket` has no structural key beyond the field groups: its primary fields render in the header row, its `groupingIdentifier` groups related passes, and its auxiliary fields are the only place where `row` applies.

## Localization

Localization is an indirection, not an overlay of the JSON tree: to localize a string, the `pass.json` value becomes a key, and that key is defined in every `pass.strings` catalog.

```json
"primaryFields": [
    { "key": "offer", "value": "OfferAmount", "label": "OfferAmountLabel" }
]
```

```
/* en.lproj/pass.strings */
"OfferAmount" = "100% off";
"OfferAmountLabel" = "Anything you want!";
```

- The localizable slots include the top-level `description`, `organizationName`, and `logoText`, and each field's `label`, `value`, `attributedValue`, and `changeMessage`.
- The lookup rule is simple: if the string matches a key in the resolved catalog, substitute the translation; otherwise use the literal string unchanged.
- Language fallback picks the catalog that matches the interface language, may strip the region from `en-US` to `en`, and finally falls back to `en.lproj`.
- Apple says to use UTF-16 encoding for non-ASCII characters, but real passes ship UTF-8, so a reader sniffs the encoding: a first byte of punctuation, a newline, or a UTF-8 byte-order mark means UTF-8, and anything else means UTF-16 big-endian.
- Apple also localizes dates and currencies automatically for values that use `dateStyle`, `timeStyle`, or `currencyCode`, even with no catalog present.

## What This Project Does With the Format

The reader in this repository implements the following parts of the format, and nothing more.

- It opens the `.pkpass` as a ZIP, honouring each member's own compression method, and verifies each member against the CRC in the central directory.
- It reads `pass.json`, including the required keys, the supported optional top-level keys, the style and `transitType`, the five classic field groups with their per-field keys, the three colours, the legacy `barcode` and the `barcodes` array, and the relevance locations.
- It reads `manifest.json` into member and digest pairs and checks every entry's SHA-1 against the member bytes, naming missing entries, mismatched entries, and members that are present but unlisted.
- It checks the detached PKCS#7 signature over `manifest.json` with `openssl` when that program is present, reporting `absent`, `verified`, `invalid`, or `unavailable`, and it names the signer from the embedded certificate.
- It returns a pass with warnings instead of refusing a structurally readable one, so that a viewer can still show it.

Two capabilities are deliberately absent, and the reference above describes more of the format than the code currently uses.

- No barcode encoder exists, so `pkpass-render` shows the first barcode's format, `altText`, and payload as text rather than drawing a symbol; the rule the renderer follows is `PASS-RENDERER-R009` in the pass renderer feature, and the project's open work is tracked in `TODO.md`.
- The reader does not resolve `.lproj` catalogs, and it does not read `additionalInfoFields`, `semantics`, `beacons`, `maxDistance`, or the poster and airline extras.

## Sources

Apple, current documentation:

- Wallet Passes — <https://developer.apple.com/documentation/walletpasses>
- `Pass` object — <https://developer.apple.com/documentation/walletpasses/pass>
- `PassFields` — <https://developer.apple.com/documentation/walletpasses/passfields>
- `PassFieldContent` — <https://developer.apple.com/documentation/walletpasses/passfieldcontent>
- `Pass.Barcode` — <https://developer.apple.com/documentation/walletpasses/pass/barcode-data.dictionary>
- `Pass.Barcodes` — <https://developer.apple.com/documentation/walletpasses/pass/barcodes-data.dictionary>
- `Pass.BoardingPass` — <https://developer.apple.com/documentation/walletpasses/pass/boardingpass-data.dictionary>
- Creating the Source for a Pass — <https://developer.apple.com/documentation/walletpasses/creating-the-source-for-a-pass>
- Building a Pass — <https://developer.apple.com/documentation/walletpasses/building-a-pass>
- Distributing and updating a pass — <https://developer.apple.com/documentation/walletpasses/distributing-and-updating-a-pass>
- Personalize a pass — <https://developer.apple.com/documentation/walletpasses/personalize>

Apple, archived documentation:

- PassKit Package Format Reference, Package Structure — <https://web.archive.org/web/20190319114620id_/https://developer.apple.com/library/archive/documentation/UserExperience/Reference/PassKit_Bundle/Chapters/PackageStructure.html>
- PassKit Package Format Reference, Top-Level Keys — <https://web.archive.org/web/2021id_/https://developer.apple.com/library/archive/documentation/UserExperience/Reference/PassKit_Bundle/Chapters/TopLevel.html>
- PassKit Package Format Reference, Lower-Level Keys — <https://web.archive.org/web/2021id_/https://developer.apple.com/library/archive/documentation/UserExperience/Reference/PassKit_Bundle/Chapters/LowerLevel.html>
- PassKit Package Format Reference, Field Dictionary Keys — <https://web.archive.org/web/20190319114642id_/https://developer.apple.com/library/archive/documentation/UserExperience/Reference/PassKit_Bundle/Chapters/FieldDictionary.html>
- Wallet Developer Guide, Pass Design and Creation — <https://web.archive.org/web/20200920184004id_/https://developer.apple.com/library/archive/documentation/UserExperience/Conceptual/PassKit_PG/Creating.html>
- Wallet Developer Guide, Rewards Enrollment — <https://web.archive.org/web/2021id_/https://developer.apple.com/library/archive/documentation/UserExperience/Conceptual/PassKit_PG/PassPersonalization.html>

Open-source readers and platform definitions:

- shared-mime-info MIME database — <https://gitlab.freedesktop.org/xdg/shared-mime-info/-/raw/master/data/freedesktop.org.xml.in>
- shared-mime-info release notes — <https://gitlab.freedesktop.org/xdg/shared-mime-info/-/raw/master/NEWS>
- KDE KPkPass source — <https://invent.kde.org/pim/kpkpass/-/blob/master/src/pass.cpp>
- GNOME Passes application — <https://github.com/pablo-s/passes>
