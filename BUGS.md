# BUGS

Items found while validating the TypeScript grammar profiles against the
tree-sitter-typescript reference grammar and the real `tsc` compiler
(see `tests/treesitter_compare/`). Each entry reproduces with the textparser
CLI over the snippet shown. Remove an entry once fixed and validated.

## JSX/TSX attribute values that are arrow functions are rejected

`<div onClick={() => go()} />` (an arrow expression as the value of a JSX
attribute) fails with `TS1005` (')' expected) at the closing `}` of the
attribute container. Both `tsc` and tree-sitter accept arrow expressions in
attribute-value expression containers.

```tsx
const el = <div onClick={() => go()} disabled>Hi {name}</div>;
```

Plain attribute expressions (`onClick={handler}`, `{true}`, `{fn()}`) parse;
children containers such as `{items.map(i => <li/>)}` also parse - only an
arrow-headed expression directly as an attribute value fails. The attribute
value scanner should attempt the arrow-head speculation used by expression
goals before the ordinary attribute expression parse.
