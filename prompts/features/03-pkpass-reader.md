# Feature: PKPASS Reader

## Purpose

The project must read an Apple Wallet pass: a `.pkpass` file is a ZIP whose
`pass.json` describes a pass, with `manifest.json` recording a SHA-1 digest per
member and `signature` carrying a detached PKCS#7 signature over the manifest.

## Requirements

- `PKPASS-READER-R001` — The reader must read a `.pkpass` as a ZIP archive, honouring each member's own compression method, both stored and deflate.
- `PKPASS-READER-R002` — The reader must verify each extracted member against the CRC in the central directory.
- `PKPASS-READER-R003` — The reader must report whether `pass.json`, `manifest.json`, and `signature` are present.
- `PKPASS-READER-R004` — The reader must fail with a message when the file is not a readable ZIP, or when `pass.json` is absent or not a JSON object.
- `PKPASS-READER-R005` — The reader must read `formatVersion` and the required top-level keys `passTypeIdentifier`, `serialNumber`, `teamIdentifier`, `organizationName`, and `description`.
- `PKPASS-READER-R006` — The reader must read the optional top-level keys `logoText`, `relevantDate`, `expirationDate`, `voided`, `groupingIdentifier`, `sharingProhibited`, `webServiceURL`, `authenticationToken`, and `nfc`.
- `PKPASS-READER-R007` — The reader must determine the pass style from exactly one of `generic`, `storeCard`, `coupon`, `eventTicket`, or `boardingPass`, and read `transitType` from a boarding pass.
- `PKPASS-READER-R008` — The reader must read the field groups `headerFields`, `primaryFields`, `secondaryFields`, `auxiliaryFields`, and `backFields`, keeping the group each field came from.
- `PKPASS-READER-R009` — The reader must read a field's `key`, `label`, `value`, `attributedValue`, `changeMessage`, `textAlignment`, `dateStyle`, `timeStyle`, `currencyCode`, `numberStyle`, and `row`.
- `PKPASS-READER-R010` — The reader must read both the legacy singular `barcode` object and the `barcodes` array, and must prefer the array when both are present.
- `PKPASS-READER-R011` — The reader must parse a colour written as `rgb(r,g,b)` or `rgba(r,g,b,a)`, including the spaces real passes put after the commas, and as `#RGB`, `#RRGGBB`, or `#AARRGGBB` where the alpha channel leads the four-channel hex form.
- `PKPASS-READER-R012` — The reader must record an unreadable colour as a warning rather than failing the pass.
- `PKPASS-READER-R013` — The reader must read `locations` entries (latitude, longitude, and relevantText).
- `PKPASS-READER-R014` — The reader must read `manifest.json` into member-name and digest pairs.
- `PKPASS-READER-R015` — The reader must list the archive's member names and identify the members that are PNG images.
- `PKPASS-READER-R016` — The verifier must check every manifest entry's SHA-1 digest against the member's bytes and report missing and mismatched entries separately.
- `PKPASS-READER-R017` — The verifier must name a member that is present in the archive but in no manifest entry.
- `PKPASS-READER-R018` — The verifier must check the detached PKCS#7 signature over `manifest.json` when `openssl` is available, and must distinguish the statuses `absent`, `verified`, `invalid`, and `unavailable`.
- `PKPASS-READER-R019` — The reader must return a pass with warnings rather than refusing a structurally readable pass, so a viewer can still show it.
- `PKPASS-READER-R020` — No public interface may throw.

## Behavior

- Given the real-world color form `rgb(86, 39, 126)`, the three channels are read as 86, 39, and 126.
- Given a pass that uses the legacy singular `barcode`, exactly one barcode is reported.
- Given a pass whose members mix stored and deflated entries, every member extracts.
- Given a member whose bytes no longer match its manifest digest, the entry is reported as mismatched and the pass still reads.
- Given a pass with no `signature` member, `hasSignature` is false and the signature status is `absent`.
- Given a file that is not a ZIP, the reader fails with a message and no partial pass.

## Dependencies

- None.
