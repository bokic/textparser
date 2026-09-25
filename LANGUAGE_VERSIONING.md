# Language Versioning

## Recommended model

Use shared language rules plus small, version-specific JSON overlays. Keep the
validator and parser code shared where possible; version files describe the
differences for a target language version.

For CFML, a possible layout is:

```text
definitions/metadata/cfml/
  cfml_common.json
  cfml_cf2021.json
  cfml_cf2023.json
```

The common file contains rules known to apply to every supported version. A
version file contains only additions, overrides, and removals for that version.
Do not put a rule in the common file until it has been checked against all
supported versions.

## Loading and merging

Load the selected version file and the common file it names, then combine their
rules in memory before validation. The version file should name its common file
explicitly, for example with a `common` property:

```json
{
  "metadataType": "version",
  "common": "cfml_common.json",
  "functions": {
    "arraynew": { "minArgs": 0, "maxArgs": 2 },
    "newfeature": { "minArgs": 1, "maxArgs": 1 }
  }
}
```

Mark the shared file explicitly too:

```json
{
  "metadataType": "common",
  "functions": {},
  "tags": {}
}
```

The Python header generator should use `metadataType` to distinguish metadata
files from standalone language definitions and skip generating a standalone
header for files marked `common`. The runtime loader must still load common
files. If using build-time generation instead, the generator can merge common
metadata into a version-specific generated header.

Merge keyed collections such as `functions` and `tags` by entry name. Entries
in the selected version replace the entire common entry with the same name;
entries not overridden are inherited. Replace complete function signatures,
including parameter arrays, rather than merging their fields. Reject malformed
entries and ambiguous duplicate definitions. If later versions need to remove a
common item, represent that explicitly in the overlay.

Support function status values such as `deprecated` and `removed` when the
target version needs them. Deprecated functions remain recognized but can emit
a warning. Removed functions are unavailable for that target version and
should produce an error when called.

## Syntax and semantic rules

Keep syntax rules distinct from semantic metadata:

- Built-in function availability and signatures, tag rules, and similar
  validation facts belong in versioned metadata.
- Operators are syntax. Version-specific operators may need an operator-table
  overlay with token/spelling, precedence, associativity, and role.
- If a syntax change also affects tokenization or grammar productions, an
  operator-table overlay alone is insufficient; use a small grammar overlay or
  version-specific grammar definition for that change.

The loader should combine the common grammar/operator data and the selected
version's syntax changes along with the validator metadata, while keeping those
sections separate in the data model.

## Runtime loading versus generated headers

Prefer runtime JSON loading when users need to select the target version
without rebuilding. In that design, the loader combines the common and selected
version data into in-memory rule tables; generated combined headers are not
needed.

Generated headers are an alternative when version choice is made at build
time. A generated header can contain the combined rules for one version, but
switching versions then requires selecting or regenerating the header and
rebuilding. Do not generate a combined header and also expect runtime JSON
selection to change the compiled rules.

The current CFML validator uses compile-time metadata tables. Adopting runtime
version selection will require moving those data tables into loadable metadata
and adding an explicit target-version selection to the parser/validator API or
configuration. The default behavior for an omitted version must be defined.

## Applying the approach to other languages

Use the same broad structure where it fits, but version the data that actually
changes for each language. Some languages may need versioned grammar or
operator tables; others may need only built-in names, signatures, or semantic
rules. Avoid duplicating full definitions when a small overlay can express the
difference clearly.
