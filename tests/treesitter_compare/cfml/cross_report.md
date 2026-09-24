# Cross-run: textparser vs tree-sitter-cfml on each other's tests

Every source below was handed to BOTH parsers: textparser (`cstdump`, CST) and tree-sitter (`dump`, cfml/cfscript/cfquery grammar per the corpus origin). See `cross.py` header for the `parity`/`shape` definitions.

## Summary

- sources tested: **456**
- parity: accept-both **317**, reject-both **3**, tp-only **1**, ts-only **135**
- shape (of accept-both pairs): same **0**, close **114**, diff **202**

| group | tests | both accept | parity breakdown |
|---|---|---|---|
| cfml-corpus | 117 | 90 | accept-both=90, tp-only=1, ts-only=26 |
| script-corpus | 164 | 78 | accept-both=78, reject-both=1, ts-only=85 |
| query-corpus | 72 | 50 | accept-both=50, ts-only=22 |
| examples | 2 | 2 | accept-both=2 |
| tp-unit | 101 | 97 | accept-both=97, reject-both=2, ts-only=2 |

## Why the accept-both pairs differ in shape (shape=diff)

Both parsers model the same constructs; the differences are node-granularity, not semantics (verified by spot-checking outlines). The skeleton comparison drops node names, so textparser's finer marker/scope nodes make identical skeletons rare.

- **148** — textparser tree has ≥ 1.6× the nodes (marker/scope/keyword nodes, e.g. `Repeat`, `*_Start`/`*_End`, `TagSelfClose`, `FunctionKeyword`; tree-sitter folds these into named nodes)
- **3** — tree-sitter tree has ≥ 1.6× the nodes (quoted hash/`#` nodes, field wrappers)
- **51** — comparable node counts, differing shape (sampled: still same construct, different node naming/granularity)

## Full table

| group | file | test | tp | tpΔ | ts | parity | shape | tp# | ts# | dp | ds |
|---|---|---|---|---|---|---|---|---|---|---|---|
| cfml-corpus | case_insensitivity.txt | 0: cf_var keyword casing | OK | 0 | OK | accept-both | diff | 34 | 19 | 6 | 3 |
| cfml-corpus | case_insensitivity.txt | 1: identifiers beginning with a keyword are not split | OK | 0 | OK | accept-both | close | 157 | 131 | 3 | 3 |
| cfml-corpus | case_insensitivity.txt | 2: keyword-prefixed identifiers as call targets and members | OK | 0 | OK | accept-both | diff | 44 | 30 | 8 | 7 |
| cfml-corpus | case_insensitivity.txt | 3: VARIABLES scope is not the var keyword | OK | 0 | OK | accept-both | diff | 36 | 25 | 7 | 4 |
| cfml-corpus | case_insensitivity.txt | 4: cffunction attribute values that look like keywords | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | case_insensitivity.txt | 5: keyword-prefixed identifiers at expression start | OK | 0 | OK | accept-both | close | 31 | 26 | 3 | 3 |
| cfml-corpus | case_insensitivity.txt | 6: keywords inside strings and hash expressions | OK | 0 | OK | accept-both | diff | 30 | 18 | 5 | 4 |
| cfml-corpus | case_insensitivity.txt | 7: uppercase value literals in cfset | OK | 0 | OK | accept-both | close | 28 | 24 | 4 | 4 |
| cfml-corpus | case_insensitivity.txt | 8: cf tag name casings | OK | 0 | OK | accept-both | diff | 28 | 13 | 4 | 3 |
| cfml-corpus | case_insensitivity.txt | 9: cf tag name casings across tag types | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cdata.txt | 0: cdata section with hash expression inside cfoutput | OK | 0 | OK | accept-both | diff | 29 | 10 | 10 | 7 |
| cfml-corpus | cdata.txt | 1: cdata section with plain text | OK | 0 | OK | accept-both | diff | 14 | 3 | 2 | 2 |
| cfml-corpus | cf_body_tags.txt | 0: cfprocessingdirective with body | OK | 0 | OK | accept-both | close | 17 | 16 | 5 | 5 |
| cfml-corpus | cf_body_tags.txt | 1: cfsetting is a void tag | OK | 0 | OK | accept-both | diff | 25 | 17 | 5 | 3 |
| cfml-corpus | cfhtml.txt | 0: cfhtml embedded expressions and cfset variants | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfhtml.txt | 1: cfhtml table, cfloop, and cf comment interaction | OK | 0 | OK | accept-both | close | 76 | 79 | 5 | 6 |
| cfml-corpus | cfhtml.txt | 2: cfsavecontent with script element | OK | 0 | OK | accept-both | diff | 33 | 14 | 7 | 5 |
| cfml-corpus | cfhtml.txt | 3: cfsavecontent with only script element | OK | 0 | OK | accept-both | diff | 25 | 12 | 7 | 5 |
| cfml-corpus | cfhtml.txt | 4: cfsavecontent without script element | OK | 0 | OK | accept-both | diff | 20 | 7 | 5 | 3 |
| cfml-corpus | cfhtml.txt | 5: inline JavaScript with less-than operator in attribute value | OK | 0 | OK | accept-both | close | 15 | 13 | 5 | 5 |
| cfml-corpus | cfhtml.txt | 6: script element with cf tags but hash as raw text | OK | 0 | OK | accept-both | diff | 29 | 11 | 7 | 3 |
| cfml-corpus | cfhtml.txt | 7: script element with cfoutput enables hash expressions | OK | 0 | OK | accept-both | close | 13 | 11 | 5 | 4 |
| cfml-corpus | cfhtml.txt | 8: script element hash without cfoutput is raw text | OK | 0 | OK | accept-both | close | 7 | 7 | 2 | 3 |
| cfml-corpus | cfhtml.txt | 9: cfsavecontent with an xml declaration | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfif_tag_attributes.txt | 0: cfif in self-closing tag attributes with ampersands and hash expressions | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 0: cfif | OK | 0 | OK | accept-both | diff | 11 | 6 | 4 | 3 |
| cfml-corpus | cfml.txt | 1: cfif with else | OK | 0 | OK | accept-both | diff | 17 | 9 | 5 | 3 |
| cfml-corpus | cfml.txt | 2: cfif with elseif | OK | 0 | OK | accept-both | diff | 20 | 12 | 6 | 5 |
| cfml-corpus | cfml.txt | 3: cfif with elseif and else | OK | 0 | OK | accept-both | diff | 26 | 15 | 6 | 5 |
| cfml-corpus | cfml.txt | 4: cffunction | OK | 0 | OK | accept-both | diff | 80 | 53 | 7 | 7 |
| cfml-corpus | cfml.txt | 5: get results | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 6: cfscript | OK | 0 | OK | accept-both | diff | 82 | 3 | 11 | 2 |
| cfml-corpus | cfml.txt | 7: nested cfif | OK | 0 | OK | accept-both | diff | 30 | 17 | 6 | 5 |
| cfml-corpus | cfml.txt | 8: / Characters used in tags and nested cfml comments | OK | 0 | OK | accept-both | close | 32 | 25 | 5 | 5 |
| cfml-corpus | cfml.txt | 9: cfscript comments | OK | 0 | OK | accept-both | diff | 29 | 3 | 8 | 2 |
| cfml-corpus | cfml.txt | 10: cfscript switch | OK | 0 | OK | accept-both | diff | 41 | 6 | 9 | 3 |
| cfml-corpus | cfml.txt | 11: cfquery with cfqueryparam in cfml | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 12: cfif with html-only branch and erroneous html end tags | OK | 0 | OK | accept-both | close | 21 | 17 | 4 | 4 |
| cfml-corpus | cfml.txt | 13: mixed cfset, cfscript, cfoutput, and complex attribute hashes | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 14: html table with nested cfloop and cfoutput attribute | OK | 0 | OK | accept-both | close | 83 | 82 | 7 | 9 |
| cfml-corpus | cfml.txt | 15: hash-empty attributes, cfif with complex call chain, and cfloop/cfset attribute hashes | OK | 0 | OK | accept-both | diff | 154 | 100 | 12 | 9 |
| cfml-corpus | cfml.txt | 16: cfoutput wrapping anchor with hash-empty and normal attributes | OK | 0 | OK | accept-both | close | 21 | 15 | 7 | 6 |
| cfml-corpus | cfml.txt | 17: deeply nested cfif chains and complex cfzip/cffile/cfzipparam usage | OK | 0 | OK | accept-both | diff | 340 | 276 | 16 | 13 |
| cfml-corpus | cfml.txt | 18: cfxml | OK | 0 | OK | accept-both | diff | 41 | 6 | 7 | 3 |
| cfml-corpus | cfml.txt | 19: cfquery | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 20: cftransaction | OK | 0 | OK | accept-both | close | 53 | 44 | 7 | 7 |
| cfml-corpus | cfml.txt | 21: var vs VARIABLES | OK | 0 | OK | accept-both | diff | 41 | 26 | 7 | 4 |
| cfml-corpus | cfml.txt | 22: static accessor | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 23: cfzip3 | OK | 0 | OK | accept-both | diff | 97 | 67 | 8 | 8 |
| cfml-corpus | cfml.txt | 24: cfset string-plus-hash expression and cfswitch inside cfquery | OK | 0 | OK | accept-both | diff | 46 | 17 | 7 | 6 |
| cfml-corpus | cfml.txt | 25: cfzip tag with mixed start/end/selfclose and embedded cfset | OK | 0 | OK | accept-both | close | 44 | 38 | 5 | 5 |
| cfml-corpus | cfml.txt | 26: cftransaction2 | OK | 0 | OK | accept-both | diff | 65 | 50 | 5 | 8 |
| cfml-corpus | cfml.txt | 27: cfoutput boolean expression with JS-style operators | OK | 0 | OK | accept-both | diff | 23 | 16 | 8 | 7 |
| cfml-corpus | cfml.txt | 28: cfloop with integers without quotes | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 29: nested calls | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 30: component ( cfscript ) | OK | 0 | OK | accept-both | diff | 348 | 3 | 22 | 2 |
| cfml-corpus | cfml.txt | 31: abstract component ( cfscript ) | OK | 0 | OK | accept-both | diff | 17 | 3 | 4 | 2 |
| cfml-corpus | cfml.txt | 32: static component ( cfscript ) | OK | 0 | OK | accept-both | diff | 17 | 3 | 4 | 2 |
| cfml-corpus | cfml.txt | 33: final component ( cfscript ) | OK | 0 | OK | accept-both | diff | 17 | 3 | 4 | 2 |
| cfml-corpus | cfml.txt | 34: FINAL component, case-insensitive ( cfscript ) | OK | 0 | OK | accept-both | diff | 17 | 3 | 4 | 2 |
| cfml-corpus | cfml.txt | 35: a modifier word that is not a component file stays text | OK | 1 | OK | accept-both | diff | 6 | 3 | 2 | 1 |
| cfml-corpus | cfml.txt | 36: nested comments | OK | 0 | OK | accept-both | - | - | - | - | - |
| cfml-corpus | cfml.txt | 37: implicit close tag with <cfelse> | OK | 0 | OK | accept-both | close | 40 | 32 | 7 | 6 |
| cfml-corpus | cfml.txt | 38: obscure test 1 | OK | 0 | OK | accept-both | diff | 582 | 489 | 20 | 12 |
| cfml-corpus | cfml.txt | 39: cfif with erroneous end tags crossing boundary | OK | 0 | OK | accept-both | close | 83 | 63 | 8 | 8 |
| cfml-corpus | cfml.txt | 40: cfset with final modifier | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 41: cfobject void tag | OK | 0 | OK | accept-both | diff | 15 | 9 | 5 | 3 |
| cfml-corpus | cfml.txt | 42: dynamic tag name with hash expression | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 43: ordered struct literal in cfscript block | OK | 0 | OK | accept-both | diff | 14 | 3 | 6 | 2 |
| cfml-corpus | cfml.txt | 44: cfcache as paired tag | OK | 0 | OK | accept-both | close | 82 | 63 | 7 | 6 |
| cfml-corpus | cfml.txt | 45: doubled-quote escape in tag attribute values | OK | 0 | OK | accept-both | diff | 57 | 36 | 5 | 4 |
| cfml-corpus | cfml.txt | 46: array type in parameter position | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 47: var-scoped dotted name in a for-in header | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 48: word-shaped operator as a parameter name behind a type | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | cfml.txt | 49: an unquoted struct as a tag attribute value is rejected (#115) | OK | 0 | ERR | tp-only | - | - | - | - | - |
| cfml-corpus | cfoutput_script.txt | 0: script element inside cfoutput enables hash expressions | OK | 0 | OK | accept-both | diff | 22 | 11 | 8 | 4 |
| cfml-corpus | cfoutput_script.txt | 1: script element without cfoutput treats hash as raw text | OK | 0 | OK | accept-both | diff | 17 | 7 | 6 | 3 |
| cfml-corpus | common.txt | 0: common: cfloop in its five forms | OK | 0 | OK | accept-both | close | 120 | 96 | 5 | 5 |
| cfml-corpus | common.txt | 1: common: cfoutput with query and group | OK | 0 | OK | accept-both | diff | 33 | 15 | 7 | 5 |
| cfml-corpus | common.txt | 2: common: word operators in a cfif | OK | 0 | OK | accept-both | close | 24 | 20 | 6 | 6 |
| cfml-corpus | common.txt | 3: common: cfswitch with delimited cfcase and default | OK | 0 | OK | accept-both | close | 34 | 32 | 5 | 6 |
| cfml-corpus | common.txt | 4: common: cflock and cfthread | OK | 0 | OK | accept-both | close | 48 | 41 | 5 | 5 |
| cfml-corpus | common.txt | 5: common: custom tags and taglib imports | OK | 0 | OK | accept-both | close | 36 | 27 | 5 | 5 |
| cfml-corpus | common.txt | 6: common: cftry with typed cfcatch, cfrethrow and cffinally | OK | 0 | OK | accept-both | close | 34 | 32 | 5 | 6 |
| cfml-corpus | common.txt | 7: common: cfinvoke with cfinvokeargument | OK | 0 | OK | accept-both | close | 35 | 26 | 5 | 5 |
| cfml-corpus | common.txt | 8: common: cfhttp with cfhttpparam | OK | 0 | OK | accept-both | close | 40 | 29 | 5 | 5 |
| cfml-corpus | common.txt | 9: common: cfmail with cfmailparam | OK | 0 | OK | accept-both | close | 38 | 27 | 7 | 5 |
| cfml-corpus | common.txt | 10: common: cfstoredproc with procparam and procresult | OK | 0 | OK | accept-both | diff | 44 | 30 | 5 | 5 |
| cfml-corpus | common.txt | 11: common: cfargument, cfparam and cfreturn in a tag component | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | common.txt | 12: common: query of queries with result and maxrows | OK | 0 | OK | accept-both | diff | 41 | 15 | 5 | 3 |
| cfml-corpus | common.txt | 13: common: cfqueryparam with a list | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | common.txt | 14: common: cfmodule, cfinclude and cfabort | OK | 0 | OK | accept-both | diff | 33 | 22 | 5 | 5 |
| cfml-corpus | common.txt | 15: common: cfheader, cfcontent and cfcookie | OK | 0 | OK | accept-both | diff | 43 | 25 | 5 | 3 |
| cfml-corpus | common.txt | 16: common: cftimer with a body | OK | 0 | OK | accept-both | close | 41 | 34 | 5 | 5 |
| cfml-corpus | common.txt | 17: common: cflogin, cfloginuser and cflogout | OK | 0 | OK | accept-both | close | 27 | 24 | 5 | 6 |
| cfml-corpus | common.txt | 18: common: cfdocument with sections and items | OK | 0 | OK | accept-both | close | 29 | 26 | 5 | 7 |
| cfml-corpus | common.txt | 19: common: cfchart with series and data | OK | 0 | OK | accept-both | close | 50 | 43 | 5 | 7 |
| cfml-corpus | common.txt | 20: common: cfform with inputs | OK | 0 | OK | accept-both | close | 70 | 60 | 5 | 7 |
| cfml-corpus | common.txt | 21: common: cfspreadsheet, cfimage and cffeed | OK | 0 | OK | accept-both | diff | 58 | 39 | 5 | 5 |
| cfml-corpus | common.txt | 22: common: cfldap, cfftp and cfpop | OK | 0 | OK | accept-both | close | 63 | 53 | 5 | 7 |
| cfml-corpus | common.txt | 23: common: cfcollection, cfindex and cfsearch | OK | 0 | OK | accept-both | close | 63 | 53 | 5 | 7 |
| cfml-corpus | common.txt | 24: common: cfwddx, cfobjectcache and cfflush | OK | 0 | OK | accept-both | close | 38 | 27 | 5 | 5 |
| cfml-corpus | common.txt | 25: common: cfassociate and cfexit in a custom tag | OK | 0 | OK | accept-both | diff | 35 | 24 | 7 | 6 |
| cfml-corpus | common.txt | 26: common: cfdbinfo and cfregistry | OK | 0 | OK | accept-both | close | 49 | 41 | 5 | 6 |
| cfml-corpus | common.txt | 27: common: deeply nested tag chains | OK | 0 | OK | accept-both | diff | 69 | 47 | 13 | 10 |
| cfml-corpus | common.txt | 28: common: a `<` that opens a run of template text | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | common.txt | 29: common: bare angle brackets in template text | OK | 0 | OK | accept-both | diff | 46 | 30 | 4 | 3 |
| cfml-corpus | common.txt | 30: common: new with a java: type prefix and a dotted java path | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | common.txt | 31: common: debugger as an ordinary identifier | NO | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | common.txt | 32: common: thin-arrow lambda | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | real_world.txt | 0: real world: dynamic closing tag name (Taffy ArrayToXML) | OK | 0 | OK | accept-both | diff | 32 | 19 | 5 | 7 |
| cfml-corpus | real_world.txt | 1: real world: javascript inside a script block (Lucee form.cfm, Mura admin) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | real_world.txt | 2: real world: param statement inside cfscript (Lucee LDEV4374) | OK | 0 | OK | accept-both | diff | 20 | 3 | 7 | 2 |
| cfml-corpus | real_world.txt | 3: real world: cfcontinue inside cfloop inside cfoutput (TestBox mintext.cfm) | OK | 0 | OK | accept-both | diff | 57 | 37 | 10 | 7 |
| cfml-corpus | real_world.txt | 4: real world: function call in a cfif condition (Lucee admin templates) | OK | 0 | OK | accept-both | diff | 36 | 24 | 9 | 5 |
| cfml-corpus | real_world.txt | 5: real world: cfquery with cfqueryparam and conditional SQL (Mura, Slatwall) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| cfml-corpus | real_world.txt | 6: real world: `new` as a variable name in a tag component (Mura fileWriter.cfc) | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 0: statement keywords are case-insensitive | OK | 0 | OK | accept-both | diff | 128 | 62 | 7 | 6 |
| script-corpus | case_insensitivity.txt | 1: mixed-case statement keywords | OK | 0 | OK | accept-both | diff | 59 | 26 | 7 | 4 |
| script-corpus | case_insensitivity.txt | 2: access modifiers are case-insensitive | OK | 1 | OK | accept-both | diff | 80 | 42 | 3 | 2 |
| script-corpus | case_insensitivity.txt | 3: identifier-capable keywords still work as identifiers | OK | 8 | OK | accept-both | close | 68 | 52 | 8 | 8 |
| script-corpus | case_insensitivity.txt | 4: identifiers beginning with a keyword are not split | OK | 0 | OK | accept-both | close | 101 | 81 | 3 | 3 |
| script-corpus | case_insensitivity.txt | 5: throw with named arguments | OK | 2 | OK | accept-both | diff | 38 | 22 | 3 | 4 |
| script-corpus | case_insensitivity.txt | 6: named arguments and pairs do not collide with switch case | OK | 2 | OK | accept-both | diff | 65 | 37 | 8 | 8 |
| script-corpus | case_insensitivity.txt | 7: keyword-prefixed identifiers at statement start | OK | 0 | OK | accept-both | close | 51 | 41 | 3 | 3 |
| script-corpus | case_insensitivity.txt | 8: component casing and component as identifier | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 9: keywords as struct keys, members and strings | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 10: new as an identifier | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 11: import and new keep their statement meanings | OK | 0 | OK | accept-both | diff | 28 | 15 | 5 | 4 |
| script-corpus | case_insensitivity.txt | 12: import as an ordinary identifier | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 13: value literals are case-insensitive | OK | 0 | OK | accept-both | close | 35 | 27 | 5 | 5 |
| script-corpus | case_insensitivity.txt | 14: value literals as identifiers and properties | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 15: required and function as case-insensitive keywords | NO | 4 | OK | ts-only | - | - | - | - | - |
| script-corpus | case_insensitivity.txt | 16: all four casing forms | OK | 0 | OK | accept-both | diff | 13 | 7 | 2 | 2 |
| script-corpus | case_insensitivity.txt | 17: camelCase keyword accepts all four forms | OK | 0 | OK | accept-both | diff | 45 | 17 | 6 | 3 |
| script-corpus | case_insensitivity.txt | 18: undefined literal casing | OK | 0 | OK | accept-both | close | 16 | 13 | 3 | 3 |
| script-corpus | case_insensitivity.txt | 19: parameter type casing | OK | 0 | OK | accept-both | diff | 118 | 68 | 6 | 4 |
| script-corpus | case_insensitivity.txt | 20: instanceOf operator casing | OK | 0 | OK | accept-both | diff | 28 | 19 | 3 | 4 |
| script-corpus | cfscript.txt | 0: cfscript comments | OK | 0 | OK | accept-both | diff | 28 | 18 | 7 | 5 |
| script-corpus | cfscript.txt | 1: cfscript | OK | 0 | OK | accept-both | diff | 118 | 69 | 10 | 6 |
| script-corpus | cfscript.txt | 2: cfscript switch | OK | 0 | OK | accept-both | diff | 32 | 20 | 6 | 6 |
| script-corpus | cfscript.txt | 3: cfscript function 1 | OK | 0 | OK | accept-both | diff | 11 | 7 | 2 | 2 |
| script-corpus | cfscript.txt | 4: cfscript function 2 | OK | 0 | OK | accept-both | diff | 30 | 18 | 5 | 4 |
| script-corpus | cfscript.txt | 5: cfscript function 3 | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 6: parameter attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 7: script tag | OK | 0 | OK | accept-both | diff | 15 | 8 | 6 | 4 |
| script-corpus | cfscript.txt | 8: component | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 9: component test | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 10: try test | OK | 0 | OK | accept-both | diff | 102 | 50 | 15 | 10 |
| script-corpus | cfscript.txt | 11: testNamedArgNull | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 12: scope member access | OK | 0 | OK | accept-both | close | 25 | 19 | 4 | 4 |
| script-corpus | cfscript.txt | 13: query test / multiline tags | OK | 0 | OK | accept-both | diff | 34 | 22 | 7 | 6 |
| script-corpus | cfscript.txt | 14: try / catch | OK | 0 | OK | accept-both | diff | 68 | 42 | 11 | 8 |
| script-corpus | cfscript.txt | 15: try / catch with string type | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 16: for in with uppercase IN | OK | 0 | OK | accept-both | diff | 10 | 5 | 2 | 2 |
| script-corpus | cfscript.txt | 17: javascript escape sequences that don't work in CF | OK | 0 | OK | accept-both | diff | 27 | 12 | 9 | 6 |
| script-corpus | cfscript.txt | 18: tag statements | OK | 1 | OK | accept-both | diff | 25 | 16 | 5 | 5 |
| script-corpus | cfscript.txt | 19: ternary / elvis operator / cfml comments / regex | OK | 0 | OK | accept-both | diff | 76 | 39 | 9 | 5 |
| script-corpus | cfscript.txt | 20: failing expressions | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 21: Lucee built-in functions (all parse as call_expression) | OK | 0 | OK | accept-both | diff | 89 | 49 | 7 | 4 |
| script-corpus | cfscript.txt | 22: Lucee operators (NOT, XOR, MOD, CT, NCT, <>) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 23: queryExecute test | OK | 0 | OK | accept-both | diff | 118 | 49 | 24 | 10 |
| script-corpus | cfscript.txt | 24: abstract component | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 25: abstract component with attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 26: component with attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 27: static component | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 28: property declarations | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 29: include statement | OK | 0 | OK | accept-both | close | 4 | 3 | 1 | 2 |
| script-corpus | cfscript.txt | 30: decrement operators | OK | 0 | OK | accept-both | close | 11 | 9 | 4 | 4 |
| script-corpus | cfscript.txt | 31: private and public as identifiers | OK | 1 | OK | accept-both | close | 6 | 5 | 2 | 3 |
| script-corpus | cfscript.txt | 32: component with accessors and property | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 33: component with identifier name | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 34: savecontent with include | OK | 0 | OK | accept-both | diff | 15 | 9 | 4 | 4 |
| script-corpus | cfscript.txt | 35: while loop with increment and decrement | OK | 0 | OK | accept-both | close | 34 | 25 | 6 | 5 |
| script-corpus | cfscript.txt | 36: interface with abstract functions | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 37: inline component followed by statements | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 38: function with metadata attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 39: multiple catch clauses with dotted type | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 40: property with multiple attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 41: function attributes on new line | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 42: multi-line property declaration | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 43: final component | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 44: function with multiple access modifiers | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 45: static initializer block | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 46: ternary with new() no constructor | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 47: query variable method chaining | OK | 0 | OK | accept-both | diff | 24 | 16 | 7 | 8 |
| script-corpus | cfscript.txt | 48: query as function return type | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 49: query as parameter type | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 50: function with bare annotation | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 51: component with multi-line bare attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 52: ordered struct literal | OK | 0 | OK | accept-both | close | 6 | 5 | 2 | 3 |
| script-corpus | cfscript.txt | 53: include with multiple attributes | OK | 0 | OK | accept-both | close | 11 | 8 | 3 | 3 |
| script-corpus | cfscript.txt | 54: import with wildcard | OK | 1 | OK | accept-both | diff | 7 | 4 | 2 | 3 |
| script-corpus | cfscript.txt | 55: multiline query tag with cachedWithin | OK | 0 | OK | accept-both | diff | 49 | 26 | 8 | 8 |
| script-corpus | cfscript.txt | 56: cfml template block | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 57: cfml template block after a statement with no semicolon | OK | 1 | OK | accept-both | diff | 16 | 23 | 2 | 6 |
| script-corpus | cfscript.txt | 58: new inline component with attributes | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 59: hash empty in string concatenation | OK | 0 | OK | accept-both | close | 13 | 11 | 4 | 5 |
| script-corpus | cfscript.txt | 60: keywords as variable names | OK | 8 | OK | accept-both | close | 57 | 41 | 2 | 4 |
| script-corpus | cfscript.txt | 61: new as property name | OK | 4 | OK | accept-both | diff | 39 | 27 | 5 | 6 |
| script-corpus | cfscript.txt | 62: pair syntax in named function arguments | OK | 1 | OK | accept-both | diff | 14 | 11 | 2 | 5 |
| script-corpus | cfscript.txt | 63: pair syntax in object literal | OK | 0 | OK | accept-both | diff | 23 | 11 | 9 | 5 |
| script-corpus | cfscript.txt | 64: pair syntax in ordered struct array | OK | 0 | OK | accept-both | diff | 23 | 11 | 9 | 5 |
| script-corpus | cfscript.txt | 65: equals syntax in named function arguments | OK | 0 | OK | accept-both | diff | 18 | 11 | 7 | 5 |
| script-corpus | cfscript.txt | 66: equals syntax in object literal | OK | 0 | OK | accept-both | diff | 23 | 11 | 9 | 5 |
| script-corpus | cfscript.txt | 67: equals syntax in array | OK | 0 | OK | accept-both | diff | 23 | 11 | 10 | 5 |
| script-corpus | cfscript.txt | 68: component as a call and as a declaration | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 69: array type in parameter position | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 70: var-scoped dotted name in a for-in header | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 71: param with the = spelling of the default | OK | 1 | OK | accept-both | diff | 73 | 42 | 5 | 4 |
| script-corpus | cfscript.txt | 72: word-shaped operator as a parameter name behind a type | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 73: untyped param shorthand | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 74: access modifier combined with final | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 75: ordered struct literal, and $ as an ordinary variable (#80) | OK | 1 | OK | accept-both | diff | 52 | 32 | 6 | 5 |
| script-corpus | cfscript.txt | 76: arrow function with an empty body (#116) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 77: a built-in type name parenthesised as an expression | OK | 0 | OK | accept-both | diff | 85 | 54 | 9 | 8 |
| script-corpus | cfscript.txt | 78: var declaration with a compound assignment | OK | 3 | OK | accept-both | diff | 102 | 52 | 9 | 7 |
| script-corpus | cfscript.txt | 79: elseif as one word | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 80: return type between the access modifiers (#117) | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 81: return type before the access modifiers | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 82: subscripted static access | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 83: default modifier on an interface method | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 84: bodyless throw and query tag statements | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 85: tag statement with a bare string argument | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 86: brace-less try body | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 87: parenthesised component attributes | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 88: name:value annotations on components and functions | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 89: colon-separated script tag call attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 90: static initialiser at the top level of a region | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 91: colon assignment to a dotted name | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 92: inline java class block | OK | 4 | OK | accept-both | diff | 200 | 97 | 8 | 6 |
| script-corpus | cfscript.txt | 93: subscript assignment is not a tag statement | OK | 0 | OK | accept-both | diff | 121 | 72 | 8 | 7 |
| script-corpus | cfscript.txt | 94: default as an ordinary identifier | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 95: switch with case and default clauses | OK | 0 | OK | accept-both | diff | 47 | 26 | 6 | 6 |
| script-corpus | cfscript.txt | 96: function listener callback (#87) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 97: function listener on a new target (#98) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 98: new requires its arguments (#98) | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 99: static type prefix (#93) | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | cfscript.txt | 100: numeric struct key in write position (#86) | OK | 0 | OK | accept-both | diff | 114 | 82 | 8 | 5 |
| script-corpus | cfscript.txt | 101: comma-less function parameters (#49) | NO | 4 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 0: common: component attributes and properties | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 1: common: closures and arrow functions | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 2: common: chained member functions | NO | 3 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 3: common: elvis and safe navigation | OK | 0 | OK | accept-both | close | 22 | 19 | 6 | 6 |
| script-corpus | common.txt | 4: common: queryExecute with params and options | OK | 0 | OK | accept-both | diff | 34 | 13 | 10 | 6 |
| script-corpus | common.txt | 5: common: try catch finally | OK | 0 | OK | accept-both | diff | 44 | 22 | 9 | 7 |
| script-corpus | common.txt | 6: common: interface declaration | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 7: common: application lifecycle component | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 8: common: compound assignment and update operators | OK | 0 | OK | accept-both | close | 34 | 27 | 3 | 3 |
| script-corpus | common.txt | 9: common: nested struct and array literals | OK | 0 | OK | accept-both | diff | 65 | 27 | 16 | 10 |
| script-corpus | common.txt | 10: common: script-syntax lock, transaction and thread | OK | 0 | OK | accept-both | diff | 66 | 38 | 8 | 7 |
| script-corpus | common.txt | 11: common: script-syntax savecontent | OK | 0 | OK | accept-both | diff | 32 | 18 | 9 | 8 |
| script-corpus | common.txt | 12: common: script-syntax loop forms | OK | 0 | OK | accept-both | diff | 104 | 63 | 10 | 9 |
| script-corpus | common.txt | 13: common: for, for-in, while and do-while | OK | 0 | OK | accept-both | diff | 83 | 41 | 8 | 6 |
| script-corpus | common.txt | 14: common: named arguments and argumentCollection | OK | 0 | OK | accept-both | diff | 46 | 26 | 8 | 5 |
| script-corpus | common.txt | 15: common: new and createObject | OK | 0 | OK | accept-both | diff | 67 | 35 | 9 | 6 |
| script-corpus | common.txt | 16: common: throw with named arguments | OK | 1 | OK | accept-both | diff | 42 | 24 | 6 | 8 |
| script-corpus | common.txt | 17: common: import and include statements | OK | 0 | OK | accept-both | diff | 14 | 7 | 5 | 3 |
| script-corpus | common.txt | 18: common: javadoc annotations before a function | OK | 0 | OK | accept-both | diff | 18 | 11 | 4 | 4 |
| script-corpus | common.txt | 19: common: hash interpolation inside strings | OK | 0 | OK | accept-both | diff | 11 | 16 | 3 | 3 |
| script-corpus | common.txt | 20: common: nested ternary | OK | 0 | OK | accept-both | close | 14 | 12 | 5 | 6 |
| script-corpus | common.txt | 21: common: abstract and final functions | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 22: common: ORM property attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 23: common: remote function attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 24: common: component metadata attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 25: common: script-syntax cfdocument and cfhttp | OK | 0 | OK | accept-both | diff | 83 | 43 | 12 | 9 |
| script-corpus | common.txt | 26: common: empty struct and empty ordered struct literals | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 27: common: typed param statement | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 28: common: unterminated queryExecute string | NO | 1 | ERR | reject-both | - | - | - | - | - |
| script-corpus | common.txt | 29: common: array return types | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 30: common: script-syntax tag calls with space-separated attributes | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 31: common: subscript as a var declaration name | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 32: common: new with a dotted java path, and the java: prefix | OK | 1 | OK | accept-both | diff | 47 | 31 | 7 | 6 |
| script-corpus | common.txt | 33: common: debugger as an ordinary identifier | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | common.txt | 34: common: thin-arrow lambda | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | real_world.txt | 0: real world: string and array slicing (Lucee LDEV4374) | OK | 4 | OK | accept-both | diff | 62 | 51 | 3 | 7 |
| script-corpus | real_world.txt | 1: real world: function named after a keyword (ColdBox Matcher, TestBox Assertion) | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | real_world.txt | 2: real world: keyword-shaped parameter types (Lucee AsynchronousEvents) | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | real_world.txt | 3: real world: static initialiser block (Lucee LDEV0280) | NO | 2 | OK | ts-only | - | - | - | - | - |
| script-corpus | real_world.txt | 4: real world: padded parameter defaults (TestBox BaseSpec) | NO | 1 | OK | ts-only | - | - | - | - | - |
| script-corpus | real_world.txt | 5: real world: javadoc block before the component (ColdBox PerformanceSuite) | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 0: simple select | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 1: hash parameter and between | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 2: top, union and limit | OK | 0 | OK | accept-both | close | 24 | 19 | 3 | 2 |
| query-corpus | cfquery.txt | 3: distinct, exists, case and indexed hash | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 4: malformed select (fault tolerant) | OK | 0 | OK | accept-both | diff | 5 | 3 | 2 | 1 |
| query-corpus | cfquery.txt | 5: limit offset and ilike | OK | 0 | OK | accept-both | close | 20 | 17 | 3 | 2 |
| query-corpus | cfquery.txt | 6: cfqueryparam with hash in attribute | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 7: insert with values and hash param | OK | 0 | OK | accept-both | diff | 37 | 17 | 9 | 4 |
| query-corpus | cfquery.txt | 8: update set and where with hash params | OK | 0 | OK | accept-both | diff | 23 | 13 | 4 | 3 |
| query-corpus | cfquery.txt | 9: delete from with hash in where | OK | 0 | OK | accept-both | diff | 19 | 11 | 5 | 4 |
| query-corpus | cfquery.txt | 10: select inner join on | OK | 0 | OK | accept-both | diff | 34 | 20 | 4 | 4 |
| query-corpus | cfquery.txt | 11: select order by desc | OK | 0 | OK | accept-both | diff | 15 | 10 | 3 | 2 |
| query-corpus | cfquery.txt | 12: select group by two columns | OK | 0 | OK | accept-both | diff | 21 | 12 | 2 | 1 |
| query-corpus | cfquery.txt | 13: where and with parenthesized or | OK | 0 | OK | accept-both | diff | 24 | 22 | 8 | 4 |
| query-corpus | cfquery.txt | 14: where in list literals | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 15: hash param in select list and alias | OK | 0 | OK | accept-both | diff | 15 | 8 | 2 | 2 |
| query-corpus | cfquery.txt | 16: sql line comment before select | OK | 0 | OK | accept-both | diff | 16 | 6 | 2 | 1 |
| query-corpus | cfquery.txt | 17: with cte select | OK | 0 | OK | accept-both | diff | 29 | 19 | 3 | 4 |
| query-corpus | cfquery.txt | 18: truncate table | OK | 0 | OK | accept-both | diff | 7 | 4 | 2 | 1 |
| query-corpus | cfquery.txt | 19: where is not null | OK | 0 | OK | accept-both | close | 13 | 11 | 3 | 2 |
| query-corpus | cfquery.txt | 20: cast as type | OK | 0 | OK | accept-both | diff | 23 | 14 | 5 | 6 |
| query-corpus | cfquery.txt | 21: bracket quoted identifiers | OK | 0 | OK | accept-both | diff | 13 | 7 | 3 | 2 |
| query-corpus | cfquery.txt | 22: order by offset fetch next | OK | 0 | OK | accept-both | close | 27 | 19 | 3 | 2 |
| query-corpus | cfquery.txt | 23: semicolon two statements | OK | 0 | OK | accept-both | close | 16 | 12 | 2 | 2 |
| query-corpus | cfquery.txt | 24: merge matched update | OK | 0 | OK | accept-both | diff | 48 | 28 | 4 | 4 |
| query-corpus | cfquery.txt | 25: exec stored procedure | OK | 0 | OK | accept-both | diff | 8 | 5 | 3 | 2 |
| query-corpus | cfquery.txt | 26: window clause | OK | 0 | OK | accept-both | diff | 28 | 16 | 2 | 2 |
| query-corpus | cfquery.txt | 27: for update skip locked | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 28: pivot clause | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 29: boolean literals | OK | 0 | OK | accept-both | diff | 16 | 11 | 3 | 2 |
| query-corpus | cfquery.txt | 30: double quoted identifier | OK | 0 | OK | accept-both | diff | 8 | 5 | 2 | 1 |
| query-corpus | cfquery.txt | 31: unpivot clause | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 32: window function over | OK | 0 | OK | accept-both | diff | 28 | 15 | 3 | 2 |
| query-corpus | cfquery.txt | 33: count star over | OK | 0 | OK | accept-both | diff | 22 | 14 | 2 | 4 |
| query-corpus | cfquery.txt | 34: merge when not matched insert | OK | 0 | OK | accept-both | diff | 60 | 34 | 5 | 4 |
| query-corpus | cfquery.txt | 35: if statement around clause | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 36: loop around clause | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 37: select with math expression | OK | 0 | OK | accept-both | diff | 20 | 13 | 4 | 3 |
| query-corpus | cfquery.txt | 38: select with case when and cfqueryparam | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 39: comparison operators with join and cfqueryparam | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 40: cfif elseif with incomplete operator fragments | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 41: cfif with standalone unmatched parens | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 42: square brackets in quoted string | OK | 0 | OK | accept-both | close | 15 | 13 | 5 | 3 |
| query-corpus | cfquery.txt | 43: cfreturn inside cfif | OK | 0 | OK | accept-both | diff | 29 | 16 | 6 | 5 |
| query-corpus | cfquery.txt | 44: cfoutput query inside cfquery | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 45: backtick quoted identifiers | OK | 0 | OK | accept-both | diff | 3 | 13 | 2 | 3 |
| query-corpus | cfquery.txt | 46: tsql at variables | OK | 0 | OK | accept-both | diff | 4 | 13 | 2 | 3 |
| query-corpus | cfquery.txt | 47: double dash inside quoted string | OK | 0 | OK | accept-both | diff | 19 | 12 | 3 | 3 |
| query-corpus | cfquery.txt | 48: single quotes inside double quoted value | OK | 0 | OK | accept-both | close | 5 | 6 | 2 | 2 |
| query-corpus | cfquery.txt | 49: sql case when inside cfcase tag | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 50: concat with cfif between operands | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | cfquery.txt | 51: select with math expression | OK | 0 | OK | accept-both | diff | 13 | 9 | 4 | 3 |
| query-corpus | cfquery.txt | 52: identifiers beginning with a sql keyword | OK | 0 | OK | accept-both | diff | 21 | 13 | 2 | 1 |
| query-corpus | cfquery.txt | 53: more identifiers beginning with a sql keyword | OK | 0 | OK | accept-both | diff | 18 | 11 | 2 | 1 |
| query-corpus | cfquery.txt | 54: exact keyword as identifier needs escaping | OK | 0 | OK | accept-both | diff | 13 | 8 | 3 | 2 |
| query-corpus | cfquery.txt | 55: sql true false null stay query identifiers | OK | 0 | OK | accept-both | diff | 21 | 19 | 5 | 2 |
| query-corpus | common.txt | 0: common: insert | OK | 0 | OK | accept-both | diff | 31 | 15 | 7 | 3 |
| query-corpus | common.txt | 1: common: update with where | OK | 0 | OK | accept-both | close | 22 | 18 | 3 | 3 |
| query-corpus | common.txt | 2: common: delete with where | OK | 0 | OK | accept-both | diff | 13 | 9 | 3 | 3 |
| query-corpus | common.txt | 3: common: subquery with exists and in | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | common.txt | 4: common: left outer join with coalesce | OK | 0 | OK | accept-both | diff | 56 | 31 | 8 | 4 |
| query-corpus | common.txt | 5: common: line and block comments with escaped quotes | OK | 0 | OK | accept-both | close | 21 | 17 | 3 | 3 |
| query-corpus | common.txt | 6: common: top with multi-column order by | OK | 0 | OK | accept-both | diff | 25 | 15 | 2 | 2 |
| query-corpus | common.txt | 7: common: cast, isnull and dateadd | OK | 0 | OK | accept-both | diff | 55 | 32 | 7 | 6 |
| query-corpus | common.txt | 8: common: union all | OK | 0 | OK | accept-both | diff | 21 | 11 | 2 | 1 |
| query-corpus | common.txt | 9: common: insert select with case expression | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | common.txt | 10: common: group by with having and ordered aggregate | OK | 0 | OK | accept-both | diff | 40 | 28 | 2 | 5 |
| query-corpus | common.txt | 11: common: bitwise operators in a predicate | OK | 0 | OK | accept-both | close | 19 | 21 | 4 | 4 |
| query-corpus | real_world.txt | 0: real world: bracket identifiers and N-prefixed literal (Mura dbUpdates) | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | real_world.txt | 1: real world: T-SQL IF NOT EXISTS guard around DDL (Mura dbUpdates) | NO | 1 | OK | ts-only | - | - | - | - | - |
| query-corpus | real_world.txt | 2: real world: hash expression in a join predicate (Mura, Slatwall) | OK | 0 | OK | accept-both | diff | 90 | 57 | 5 | 5 |
| query-corpus | real_world.txt | 3: real world: common table expression and case expression (qb, Preside) | NO | 1 | OK | ts-only | - | - | - | - | - |
| examples | deeply-nested-custom.cfm | deeply-nested-custom.cfm | OK | 0 | OK | accept-both | diff | 3605 | 4806 | 2 | 1203 |
| examples | deeply-nested.cfm | deeply-nested.cfm | OK | 0 | OK | accept-both | diff | 5945 | 7926 | 2 | 1983 |
| tp-unit | cfml_tests.cpp | 0 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 1 | OK | 0 | OK | accept-both | diff | 11 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 2 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 3 | OK | 0 | OK | accept-both | close | 7 | 10 | 3 | 4 |
| tp-unit | cfml_tests.cpp | 4 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 5 | OK | 0 | OK | accept-both | close | 7 | 8 | 3 | 4 |
| tp-unit | cfml_tests.cpp | 6 | ERROR | 1 | ERR | reject-both | - | - | - | - | - |
| tp-unit | cfml_tests.cpp | 7 | OK | 0 | OK | accept-both | close | 7 | 9 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 8 | OK | 0 | OK | accept-both | close | 7 | 10 | 3 | 5 |
| tp-unit | cfml_tests.cpp | 9 | OK | 0 | OK | accept-both | diff | 7 | 11 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 10 | OK | 0 | OK | accept-both | close | 10 | 8 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 11 | ERROR | 1 | ERR | reject-both | - | - | - | - | - |
| tp-unit | cfml_tests.cpp | 12 | OK | 0 | OK | accept-both | diff | 7 | 12 | 3 | 6 |
| tp-unit | cfml_tests.cpp | 13 | OK | 0 | OK | accept-both | diff | 16 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 14 | OK | 0 | OK | accept-both | diff | 34 | 3 | 13 | 2 |
| tp-unit | cfml_tests.cpp | 15 | OK | 0 | OK | accept-both | diff | 23 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 16 | OK | 0 | OK | accept-both | diff | 17 | 3 | 8 | 2 |
| tp-unit | cfml_tests.cpp | 17 | OK | 0 | OK | accept-both | diff | 17 | 3 | 8 | 2 |
| tp-unit | cfml_tests.cpp | 18 | OK | 0 | OK | accept-both | diff | 12 | 6 | 5 | 4 |
| tp-unit | cfml_tests.cpp | 19 | OK | 0 | OK | accept-both | close | 8 | 6 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 20 | OK | 0 | OK | accept-both | diff | 10 | 6 | 4 | 3 |
| tp-unit | cfml_tests.cpp | 21 | OK | 0 | OK | accept-both | diff | 21 | 8 | 6 | 4 |
| tp-unit | cfml_tests.cpp | 22 | OK | 0 | OK | accept-both | close | 17 | 14 | 6 | 4 |
| tp-unit | cfml_tests.cpp | 23 | OK | 0 | OK | accept-both | diff | 27 | 6 | 6 | 3 |
| tp-unit | cfml_tests.cpp | 24 | OK | 0 | OK | accept-both | close | 13 | 11 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 25 | OK | 0 | OK | accept-both | close | 10 | 8 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 26 | OK | 0 | OK | accept-both | close | 10 | 9 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 27 | OK | 0 | OK | accept-both | close | 9 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 28 | OK | 0 | OK | accept-both | close | 9 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 29 | OK | 0 | OK | accept-both | close | 9 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 30 | OK | 0 | OK | accept-both | close | 11 | 11 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 31 | OK | 0 | OK | accept-both | diff | 29 | 15 | 11 | 7 |
| tp-unit | cfml_tests.cpp | 32 | OK | 0 | OK | accept-both | close | 11 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 33 | OK | 0 | OK | accept-both | close | 10 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 34 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 35 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 36 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 37 | OK | 0 | OK | accept-both | close | 26 | 21 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 38 | OK | 0 | OK | accept-both | diff | 31 | 21 | 7 | 6 |
| tp-unit | cfml_tests.cpp | 39 | OK | 0 | OK | accept-both | diff | 37 | 6 | 7 | 3 |
| tp-unit | cfml_tests.cpp | 40 | OK | 0 | OK | accept-both | close | 58 | 41 | 9 | 7 |
| tp-unit | cfml_tests.cpp | 41 | OK | 0 | OK | accept-both | close | 26 | 21 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 42 | OK | 0 | OK | accept-both | close | 31 | 22 | 7 | 6 |
| tp-unit | cfml_tests.cpp | 43 | OK | 0 | OK | accept-both | close | 28 | 24 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 44 | OK | 0 | OK | accept-both | close | 23 | 18 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 45 | OK | 0 | OK | accept-both | close | 19 | 15 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 46 | OK | 0 | OK | accept-both | diff | 14 | 7 | 5 | 3 |
| tp-unit | cfml_tests.cpp | 47 | OK | 0 | OK | accept-both | diff | 21 | 9 | 6 | 5 |
| tp-unit | cfml_tests.cpp | 48 | OK | 0 | OK | accept-both | diff | 27 | 17 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 49 | OK | 0 | OK | accept-both | close | 37 | 30 | 7 | 6 |
| tp-unit | cfml_tests.cpp | 50 | OK | 0 | OK | accept-both | close | 37 | 26 | 7 | 7 |
| tp-unit | cfml_tests.cpp | 51 | OK | 0 | OK | accept-both | close | 26 | 21 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 52 | OK | 0 | OK | accept-both | close | 13 | 13 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 53 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 54 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 55 | OK | 0 | OK | accept-both | close | 6 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 56 | OK | 0 | OK | accept-both | close | 9 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 57 | OK | 0 | OK | accept-both | close | 9 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 58 | OK | 0 | OK | accept-both | close | 10 | 11 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 59 | OK | 0 | OK | accept-both | close | 10 | 11 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 60 | OK | 0 | OK | accept-both | close | 10 | 11 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 61 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 62 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 63 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 64 | OK | 0 | OK | accept-both | close | 8 | 8 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 65 | OK | 0 | OK | accept-both | close | 8 | 9 | 4 | 4 |
| tp-unit | cfml_tests.cpp | 66 | OK | 0 | OK | accept-both | close | 12 | 10 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 67 | OK | 0 | OK | accept-both | diff | 16 | 3 | 8 | 2 |
| tp-unit | cfml_tests.cpp | 68 | OK | 0 | OK | accept-both | diff | 18 | 11 | 9 | 5 |
| tp-unit | cfml_tests.cpp | 69 | OK | 0 | OK | accept-both | diff | 11 | 6 | 5 | 4 |
| tp-unit | cfml_tests.cpp | 70 | OK | 0 | OK | accept-both | diff | 25 | 3 | 8 | 2 |
| tp-unit | cfml_tests.cpp | 71 | OK | 0 | OK | accept-both | diff | 17 | 3 | 7 | 2 |
| tp-unit | cfml_tests.cpp | 72 | OK | 0 | OK | accept-both | diff | 13 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 73 | OK | 0 | OK | accept-both | diff | 26 | 3 | 7 | 2 |
| tp-unit | cfml_tests.cpp | 74 | OK | 0 | OK | accept-both | diff | 26 | 3 | 10 | 2 |
| tp-unit | cfml_tests.cpp | 75 | OK | 0 | OK | accept-both | diff | 22 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 76 | OK | 0 | OK | accept-both | diff | 14 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 77 | OK | 0 | OK | accept-both | diff | 14 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 78 | OK | 0 | OK | accept-both | diff | 13 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 79 | OK | 0 | OK | accept-both | diff | 12 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 80 | OK | 0 | OK | accept-both | diff | 12 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 81 | OK | 0 | OK | accept-both | diff | 14 | 3 | 6 | 2 |
| tp-unit | cfml_tests.cpp | 82 | OK | 0 | OK | accept-both | close | 6 | 6 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 83 | OK | 0 | OK | accept-both | diff | 16 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 84 | OK | 0 | OK | accept-both | diff | 34 | 3 | 9 | 2 |
| tp-unit | cfml_tests.cpp | 85 | OK | 0 | OK | accept-both | diff | 49 | 3 | 11 | 2 |
| tp-unit | cfml_tests.cpp | 86 | OK | 0 | OK | accept-both | diff | 45 | 3 | 9 | 2 |
| tp-unit | cfml_tests.cpp | 87 | OK | 0 | OK | accept-both | diff | 38 | 3 | 7 | 2 |
| tp-unit | cfml_tests.cpp | 88 | OK | 0 | OK | accept-both | diff | 22 | 3 | 9 | 2 |
| tp-unit | cfml_tests.cpp | 89 | OK | 0 | OK | accept-both | close | 15 | 11 | 6 | 5 |
| tp-unit | cfml_tests.cpp | 90 | OK | 0 | OK | accept-both | diff | 16 | 3 | 7 | 2 |
| tp-unit | cfml_tests.cpp | 91 | ERROR | 1 | OK | ts-only | - | - | - | - | - |
| tp-unit | cfml_tests.cpp | 92 | NO | 1 | OK | ts-only | - | - | - | - | - |
| tp-unit | cfml_tests.cpp | 93 | OK | 0 | OK | accept-both | diff | 27 | 3 | 5 | 2 |
| tp-unit | cfml_tests.cpp | 94 | OK | 0 | OK | accept-both | diff | 75 | 3 | 10 | 2 |
| tp-unit | cfml_tests.cpp | 95 | OK | 0 | OK | accept-both | diff | 63 | 3 | 11 | 2 |
| tp-unit | cfml_tests.cpp | 96 | OK | 0 | OK | accept-both | diff | 44 | 3 | 10 | 2 |
| tp-unit | cfml_tests.cpp | 97 | OK | 0 | OK | accept-both | diff | 69 | 3 | 11 | 2 |
| tp-unit | cfml_tests.cpp | 98 | OK | 0 | OK | accept-both | close | 23 | 20 | 5 | 5 |
| tp-unit | cfml_tests.cpp | 99 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |
| tp-unit | cfml_tests.cpp | 100 | OK | 0 | OK | accept-both | close | 7 | 7 | 3 | 3 |

## A. tree-sitter accepts, textparser rejects (ts-only)

- **cfml-corpus / case_insensitivity.txt / 4: cffunction attribute values that look like keywords** — tp `NO` (1 diag)
- **cfml-corpus / case_insensitivity.txt / 9: cf tag name casings across tag types** — tp `NO` (1 diag)
- **cfml-corpus / cfhtml.txt / 0: cfhtml embedded expressions and cfset variants** — tp `NO` (1 diag)
- **cfml-corpus / cfhtml.txt / 9: cfsavecontent with an xml declaration** — tp `ERROR` (1 diag)
- **cfml-corpus / cfif_tag_attributes.txt / 0: cfif in self-closing tag attributes with ampersands and hash expressions** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 5: get results** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 11: cfquery with cfqueryparam in cfml** — tp `ERROR` (1 diag)
- **cfml-corpus / cfml.txt / 13: mixed cfset, cfscript, cfoutput, and complex attribute hashes** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 19: cfquery** — tp `ERROR` (1 diag)
- **cfml-corpus / cfml.txt / 22: static accessor** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 28: cfloop with integers without quotes** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 29: nested calls** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 40: cfset with final modifier** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 42: dynamic tag name with hash expression** — tp `ERROR` (1 diag)
- **cfml-corpus / cfml.txt / 46: array type in parameter position** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 47: var-scoped dotted name in a for-in header** — tp `NO` (1 diag)
- **cfml-corpus / cfml.txt / 48: word-shaped operator as a parameter name behind a type** — tp `NO` (1 diag)
- **cfml-corpus / common.txt / 11: common: cfargument, cfparam and cfreturn in a tag component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **cfml-corpus / common.txt / 13: common: cfqueryparam with a list** — tp `NO` (1 diag)
- **cfml-corpus / common.txt / 28: common: a `<` that opens a run of template text** — tp `NO` (1 diag)
- **cfml-corpus / common.txt / 30: common: new with a java: type prefix and a dotted java path** — tp `NO` (1 diag)
- **cfml-corpus / common.txt / 31: common: debugger as an ordinary identifier** — tp `NO` (1 diag)
- **cfml-corpus / common.txt / 32: common: thin-arrow lambda** — tp `ERROR` (1 diag)
- **cfml-corpus / real_world.txt / 1: real world: javascript inside a script block (Lucee form.cfm, Mura admin)** — tp `ERROR` (1 diag)
- **cfml-corpus / real_world.txt / 5: real world: cfquery with cfqueryparam and conditional SQL (Mura, Slatwall)** — tp `ERROR` (1 diag)
- **cfml-corpus / real_world.txt / 6: real world: `new` as a variable name in a tag component (Mura fileWriter.cfc)** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / case_insensitivity.txt / 8: component casing and component as identifier** — tp `NO` (3 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / case_insensitivity.txt / 9: keywords as struct keys, members and strings** — tp `ERROR` (1 diag)
- **script-corpus / case_insensitivity.txt / 10: new as an identifier** — tp `ERROR` (1 diag)
- **script-corpus / case_insensitivity.txt / 12: import as an ordinary identifier** — tp `ERROR` (1 diag)
- **script-corpus / case_insensitivity.txt / 14: value literals as identifiers and properties** — tp `ERROR` (1 diag)
- **script-corpus / case_insensitivity.txt / 15: required and function as case-insensitive keywords** — tp `NO` (4 diag)
- **script-corpus / cfscript.txt / 5: cfscript function 3** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 6: parameter attributes** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 8: component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 9: component test** — tp `NO` (2 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 11: testNamedArgNull** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 15: try / catch with string type** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 20: failing expressions** — tp `NO` (3 diag)
- **script-corpus / cfscript.txt / 22: Lucee operators (NOT, XOR, MOD, CT, NCT, <>)** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 24: abstract component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 25: abstract component with attributes** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 26: component with attributes** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 27: static component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 28: property declarations** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 32: component with accessors and property** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 33: component with identifier name** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 36: interface with abstract functions** — tp `NO` (2 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 37: inline component followed by statements** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 38: function with metadata attributes** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 39: multiple catch clauses with dotted type** — tp `NO` (3 diag)
- **script-corpus / cfscript.txt / 40: property with multiple attributes** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 41: function attributes on new line** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 42: multi-line property declaration** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 43: final component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 44: function with multiple access modifiers** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 45: static initializer block** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 46: ternary with new() no constructor** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 48: query as function return type** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 49: query as parameter type** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 50: function with bare annotation** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 51: component with multi-line bare attributes** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 56: cfml template block** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 58: new inline component with attributes** — tp `NO` (2 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 68: component as a call and as a declaration** — tp `NO` (2 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 69: array type in parameter position** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 70: var-scoped dotted name in a for-in header** — tp `NO` (3 diag)
- **script-corpus / cfscript.txt / 72: word-shaped operator as a parameter name behind a type** — tp `NO` (3 diag)
- **script-corpus / cfscript.txt / 73: untyped param shorthand** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 74: access modifier combined with final** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 76: arrow function with an empty body (#116)** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 79: elseif as one word** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 80: return type between the access modifiers (#117)** — tp `NO` (3 diag)
- **script-corpus / cfscript.txt / 81: return type before the access modifiers** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 82: subscripted static access** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 83: default modifier on an interface method** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 84: bodyless throw and query tag statements** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 85: tag statement with a bare string argument** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 86: brace-less try body** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 87: parenthesised component attributes** — tp `ERROR` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 88: name:value annotations on components and functions** — tp `NO` (3 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / cfscript.txt / 89: colon-separated script tag call attributes** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 90: static initialiser at the top level of a region** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 91: colon assignment to a dotted name** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 94: default as an ordinary identifier** — tp `NO` (1 diag)
- **script-corpus / cfscript.txt / 96: function listener callback (#87)** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 97: function listener on a new target (#98)** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 98: new requires its arguments (#98)** — tp `ERROR` (1 diag)
- **script-corpus / cfscript.txt / 99: static type prefix (#93)** — tp `NO` (2 diag)
- **script-corpus / cfscript.txt / 101: comma-less function parameters (#49)** — tp `NO` (4 diag)
- **script-corpus / common.txt / 0: common: component attributes and properties** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / common.txt / 1: common: closures and arrow functions** — tp `ERROR` (1 diag)
- **script-corpus / common.txt / 2: common: chained member functions** — tp `NO` (3 diag)
- **script-corpus / common.txt / 6: common: interface declaration** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / common.txt / 7: common: application lifecycle component** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / common.txt / 21: common: abstract and final functions** — tp `NO` (1 diag)
- **script-corpus / common.txt / 22: common: ORM property attributes** — tp `NO` (1 diag)
- **script-corpus / common.txt / 23: common: remote function attributes** — tp `NO` (1 diag)
- **script-corpus / common.txt / 24: common: component metadata attributes** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **script-corpus / common.txt / 26: common: empty struct and empty ordered struct literals** — tp `ERROR` (1 diag)
- **script-corpus / common.txt / 27: common: typed param statement** — tp `NO` (1 diag)
- **script-corpus / common.txt / 29: common: array return types** — tp `NO` (2 diag)
- **script-corpus / common.txt / 30: common: script-syntax tag calls with space-separated attributes** — tp `NO` (1 diag)
- **script-corpus / common.txt / 31: common: subscript as a var declaration name** — tp `NO` (2 diag)
- **script-corpus / common.txt / 33: common: debugger as an ordinary identifier** — tp `NO` (1 diag)
- **script-corpus / common.txt / 34: common: thin-arrow lambda** — tp `ERROR` (1 diag)
- **script-corpus / real_world.txt / 1: real world: function named after a keyword (ColdBox Matcher, TestBox Assertion)** — tp `NO` (1 diag)
- **script-corpus / real_world.txt / 2: real world: keyword-shaped parameter types (Lucee AsynchronousEvents)** — tp `NO` (1 diag)
- **script-corpus / real_world.txt / 3: real world: static initialiser block (Lucee LDEV0280)** — tp `NO` (2 diag)
- **script-corpus / real_world.txt / 4: real world: padded parameter defaults (TestBox BaseSpec)** — tp `NO` (1 diag)
- **script-corpus / real_world.txt / 5: real world: javadoc block before the component (ColdBox PerformanceSuite)** — tp `NO` (1 diag) — likely textparser limitation (component/interface/etc.)
- **query-corpus / cfquery.txt / 0: simple select** — tp `ERROR` (1 diag)
- **query-corpus / cfquery.txt / 1: hash parameter and between** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 3: distinct, exists, case and indexed hash** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 6: cfqueryparam with hash in attribute** — tp `ERROR` (1 diag)
- **query-corpus / cfquery.txt / 14: where in list literals** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 27: for update skip locked** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 28: pivot clause** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 31: unpivot clause** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 35: if statement around clause** — tp `ERROR` (1 diag)
- **query-corpus / cfquery.txt / 36: loop around clause** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 38: select with case when and cfqueryparam** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 39: comparison operators with join and cfqueryparam** — tp `ERROR` (1 diag)
- **query-corpus / cfquery.txt / 40: cfif elseif with incomplete operator fragments** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 41: cfif with standalone unmatched parens** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 44: cfoutput query inside cfquery** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 49: sql case when inside cfcase tag** — tp `NO` (1 diag)
- **query-corpus / cfquery.txt / 50: concat with cfif between operands** — tp `ERROR` (1 diag)
- **query-corpus / common.txt / 3: common: subquery with exists and in** — tp `NO` (1 diag)
- **query-corpus / common.txt / 9: common: insert select with case expression** — tp `NO` (1 diag)
- **query-corpus / real_world.txt / 0: real world: bracket identifiers and N-prefixed literal (Mura dbUpdates)** — tp `NO` (1 diag)
- **query-corpus / real_world.txt / 1: real world: T-SQL IF NOT EXISTS guard around DDL (Mura dbUpdates)** — tp `NO` (1 diag)
- **query-corpus / real_world.txt / 3: real world: common table expression and case expression (qb, Preside)** — tp `NO` (1 diag)
- **tp-unit / cfml_tests.cpp / 91** — tp `ERROR` (1 diag)
- **tp-unit / cfml_tests.cpp / 92** — tp `NO` (1 diag)

## B. textparser accepts, tree-sitter rejects (tp-only)

- **cfml-corpus / cfml.txt / 49: an unquoted struct as a tag attribute value is rejected (#115)** — ts root ERROR; tp `OK`

## C. both accept but structurally different (shape=diff)

### cfml-corpus / case_insensitivity.txt / 0: cf_var keyword casing

```
<cfset var a = 1>
<cfset VAR b = 2>
<cfset Var c = 3>
```

**textparser CST** `tp=34 nodes`

```
Template
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            Number
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            Number
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            Number
    TagEnd
```

**tree-sitter CST** `ts=19 nodes`

```
  program
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        number
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        number
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        number
      cf_selfclose_void_tag_end
```

### cfml-corpus / case_insensitivity.txt / 2: keyword-prefixed identifiers as call targets and members

```
<cfset model = functionalImpactModel(context=ARGUMENTS.context)>
<cfset var functionalImpactData = structNew()>
<cfset returnValue = obj.functionalImpact()>
```

**textparser CST** `tp=44 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            AssignOperator
              Variable
              PostfixExpressionSuffix
                Variable
                ObjectMember
                Variable
        RParen
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
    TagEnd
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        LParen
        RParen
    TagEnd
```

**tree-sitter CST** `ts=30 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            assignment_expression
              identifier
              member_expression
                identifier
                property_identifier
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
    cf_set_tag
      assignment_expression
        identifier
        call_expression
          member_expression
            identifier
            property_identifier
          arguments
      cf_selfclose_void_tag_end
```

### cfml-corpus / case_insensitivity.txt / 3: VARIABLES scope is not the var keyword

```
<cfset VARIABLES.test = StructNew()>
<cfset variables.other = 1>
<cfset var test = StructNew()>
```

**textparser CST** `tp=36 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        LParen
        RParen
    TagEnd
  SetTag
    SetStartTag_Start
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      Number
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
    TagEnd
```

**tree-sitter CST** `ts=25 nodes`

```
  program
    cf_set_tag
      assignment_expression
        member_expression
          identifier
          property_identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
    cf_set_tag
      assignment_expression
        member_expression
          identifier
          property_identifier
        number
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
```

### cfml-corpus / case_insensitivity.txt / 6: keywords inside strings and hash expressions

```
<cfset x = "a function b">
<cfoutput>#functionalImpact#</cfoutput>
<cfif returnValue GT 1><cfset y = 1></cfif>
```

**textparser CST** `tp=30 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      DoubleString
    TagEnd
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
    OutputEndTag
  IfTagPair
    IfStartTag_Start
    CompareOperator
      Variable
      Number
    TagEnd
    Repeat
      SetTag
        SetStartTag_Start
        AssignOperator
          Variable
          Number
        TagEnd
    IfEndTag
```

**tree-sitter CST** `ts=18 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        string
      cf_selfclose_void_tag_end
    cf_output_tag
      hash_expression
        identifier
    cf_if_tag
      binary_expression
        identifier
        number
      cf_set_tag
        assignment_expression
          identifier
          number
        cf_selfclose_void_tag_end
```

### cfml-corpus / case_insensitivity.txt / 8: cf tag name casings

```
<cfif x><cfelse></cfif>
<Cfif x><Cfelse></Cfif>
<CFIF x><CFELSE></CFIF>
```

**textparser CST** `tp=28 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    Variable
    TagEnd
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
    IfEndTag
  IfTagPair
    IfStartTag_Start
    Variable
    TagEnd
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
    IfEndTag
  IfTagPair
    IfStartTag_Start
    Variable
    TagEnd
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
    IfEndTag
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    cf_if_tag
      identifier
      cf_if_alt
        cf_else_tag
    cf_if_tag
      identifier
      cf_if_alt
        cf_else_tag
    cf_if_tag
      identifier
      cf_if_alt
        cf_else_tag
```

### cfml-corpus / cdata.txt / 0: cdata section with hash expression inside cfoutput

```
<cfoutput><![CDATA[#XMLFormat(result.addr1)#]]></cfoutput>
```

**textparser CST** `tp=29 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      CompareOperator
      LogicalNotOperator
      LBracket
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LBracket
          SharpExpression
            SharpExpression_Start
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
              RParen
            SharpExpression_Start
          RBracket
      RBracket
      CompareOperator
    OutputEndTag
```

**tree-sitter CST** `ts=10 nodes`

```
  program
    cf_output_tag
      cdata_section
        hash_expression
          call_expression
            identifier
            arguments
              member_expression
                identifier
                property_identifier
```

### cfml-corpus / cdata.txt / 1: cdata section with plain text

```
<![CDATA[plain text]]>
```

**textparser CST** `tp=14 nodes`

```
Template
  CompareOperator
  LogicalNotOperator
  LBracket
  ExpressionStatement
    Variable
  LBracket
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  RBracket
  RBracket
  CompareOperator
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cdata_section
      cdata_text
```

### cfml-corpus / cf_body_tags.txt / 1: cfsetting is a void tag

```
<cfsetting enablecfoutputonly="yes">
<p>test</p>
<cfsetting enablecfoutputonly="no">
```

**textparser CST** `tp=25 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  ExpressionStatement
    Variable
  EndTag
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    element
      start_tag
        tag_name
      html_text
      end_tag
        tag_name
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
```

### cfml-corpus / cfhtml.txt / 2: cfsavecontent with script element

```
<cfsavecontent variable="html">
some text
<script>var x = 1;</script>
more text
</cfsavecontent>
```

**textparser CST** `tp=33 nodes`

```
Template
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      StartTag
        StartTag_Start
        TagEnd
      VariableDeclarationStatement
        VarKeyword
        VariableDeclaratorList
          VariableDeclarator
            Variable
            Sequence
              AssignOperator
              Number
        Semicolon
      EndTag
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
    SavecontentEndTag
```

**tree-sitter CST** `ts=14 nodes`

```
  program
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        html_text
        script_element
          start_tag
            tag_name
          script_text
          end_tag
            tag_name
        html_text
```

### cfml-corpus / cfhtml.txt / 3: cfsavecontent with only script element

```
<cfsavecontent variable="html"><script>alert('hi');</script></cfsavecontent>
```

**textparser CST** `tp=25 nodes`

```
Template
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        TagEnd
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              SingleString
          RParen
        Semicolon
      EndTag
    SavecontentEndTag
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        script_element
          start_tag
            tag_name
          script_text
          end_tag
            tag_name
```

### cfml-corpus / cfhtml.txt / 4: cfsavecontent without script element

```
<cfsavecontent variable="html">just plain text content</cfsavecontent>
```

**textparser CST** `tp=20 nodes`

```
Template
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
    SavecontentEndTag
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        html_text
```

### cfml-corpus / cfhtml.txt / 6: script element with cf tags but hash as raw text

```
<script>
var x = '#myVar#';
<cfif isAdmin>
var admin = true;
</cfif>
</script>
```

**textparser CST** `tp=29 nodes`

```
Template
  StartTag
    StartTag_Start
    TagEnd
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          SingleString
    Semicolon
  IfTagPair
    IfStartTag_Start
    Variable
    TagEnd
    Repeat
      VariableDeclarationStatement
        VarKeyword
        VariableDeclaratorList
          VariableDeclarator
            Variable
            Sequence
              AssignOperator
              Boolean
        Semicolon
    IfEndTag
  EndTag
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    script_element
      start_tag
        tag_name
      script_text
      cf_if_tag
        identifier
        html_text
      script_text
      end_tag
        tag_name
```

### cfml-corpus / cfml.txt / 0: cfif

```
<cfif z EQ b>
    test
</cfif>
```

**textparser CST** `tp=11 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      Variable
      Variable
    TagEnd
    Repeat
      ExpressionStatement
        Variable
    IfEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_if_tag
      binary_expression
        identifier
        identifier
      html_text
```

### cfml-corpus / cfml.txt / 1: cfif with else

```
<cfif z EQ b>Yes<cfelse>Noo</cfif>
```

**textparser CST** `tp=17 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      Variable
      Variable
    TagEnd
    Repeat
      Boolean
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
      Repeat
        ExpressionStatement
          Variable
    IfEndTag
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    cf_if_tag
      binary_expression
        identifier
        identifier
      html_text
      cf_if_alt
        cf_else_tag
        html_text
```

### cfml-corpus / cfml.txt / 2: cfif with elseif

```
<cfif z EQ b>Yes<cfelseif z EQ b>No</cfif>
```

**textparser CST** `tp=20 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      Variable
      Variable
    TagEnd
    Repeat
      Boolean
    Repeat
      Sequence
        ElseIfTag
          ElseIfStartTag_Start
          CompareOperator
            Variable
            Variable
          TagEnd
        Repeat
          Boolean
    IfEndTag
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    cf_if_tag
      binary_expression
        identifier
        identifier
      html_text
      cf_if_alt
        cf_elseif_tag
          binary_expression
            identifier
            identifier
        html_text
```

### cfml-corpus / cfml.txt / 3: cfif with elseif and else

```
<cfif z EQ b>Yes<cfelseif z EQ b>No<cfelse>No</cfif>
```

**textparser CST** `tp=26 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      Variable
      Variable
    TagEnd
    Repeat
      Boolean
    Repeat
      Sequence
        ElseIfTag
          ElseIfStartTag_Start
          CompareOperator
            Variable
            Variable
          TagEnd
        Repeat
          Boolean
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
      Repeat
        Boolean
    IfEndTag
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    cf_if_tag
      binary_expression
        identifier
        identifier
      html_text
      cf_if_alt
        cf_elseif_tag
          binary_expression
            identifier
            identifier
        html_text
        cf_if_alt
          cf_else_tag
          html_text
```

### cfml-corpus / cfml.txt / 4: cffunction

```

<cffunction name="Test">

    #testAND AND ANDtestOR#

    #test AND test#

    <cfset test = "test">

    <cfif test.test EQ test>

    </cfif>

    <cfset test = test[test]>
    <cfset test = test(test).test>
    <cfset test = test(test)[test]>

</cffunction>
```

**textparser CST** `tp=80 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  SharpExpression
    SharpExpression_Start
    LogicalAndOperator
      Variable
      Variable
    SharpExpression_Start
  SharpExpression
    SharpExpression_Start
    LogicalAndOperator
      Variable
      Variable
    SharpExpression_Start
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      DoubleString
    TagEnd
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      Variable
    TagEnd
    IfEndTag
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LBracket
        Variable
        RBracket
    TagEnd
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
          RParen
        ObjectMember
        Variable
    TagEnd
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
          RParen
        LBracket
        Variable
        RBracket
    TagEnd
  EndTag
```

**tree-sitter CST** `ts=53 nodes`

```
  program
    cf_function_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      html_text
      hash_expression
        binary_expression
          identifier
          identifier
      html_text
      hash_expression
        binary_expression
          identifier
          identifier
      cf_set_tag
        assignment_expression
          identifier
          string
        cf_selfclose_void_tag_end
      cf_if_tag
        binary_expression
          member_expression
            identifier
            property_identifier
          identifier
      cf_set_tag
        assignment_expression
          identifier
          subscript_expression
            identifier
            identifier
        cf_selfclose_void_tag_end
      cf_set_tag
        assignment_expression
          identifier
          member_expression
            call_expression
              identifier
              arguments
                identifier
            property_identifier
        cf_selfclose_void_tag_end
      cf_set_tag
        assignment_expression
          identifier
          subscript_expression
            call_expression
              identifier
              arguments
                identifier
            identifier
        cf_selfclose_void_tag_end
```

### cfml-corpus / cfml.txt / 6: cfscript

```
<cfscript>

  cars = fn("abc,test","abc");

  cars = fn(test="abc",abc="test",abc);

  cars = fn["Test"](test="abc",abc="test");

  test().test();

</cfscript>
```

**textparser CST** `tp=82 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                DoubleString
              Repeat
                Sequence
                  Separator
                  Argument
                    DoubleString
            RParen
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                AssignOperator
                  Variable
                  DoubleString
              Repeat
                Sequence
                  Separator
                  Argument
                    AssignOperator
                      Variable
                      DoubleString
                Sequence
                  Separator
                  Argument
                    Variable
            RParen
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              LBracket
              DoubleString
              RBracket
            LParen
            ArgumentList
              Argument
                AssignOperator
                  Variable
                  DoubleString
              Repeat
                Sequence
                  Separator
                  Argument
                    AssignOperator
                      Variable
                      DoubleString
            RParen
        Semicolon
      ExpressionStatement
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
            ObjectMember
            Variable
          LParen
          RParen
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### cfml-corpus / cfml.txt / 7: nested cfif

```

<cfif (true)>

  test

  <cfif (true AND true)>
      test
  </cfif>

  #test#

  ##

</cfif>
```

**textparser CST** `tp=30 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    ParenthesizedExpression
      LParen
      Boolean
      RParen
    TagEnd
    Repeat
      ExpressionStatement
        Variable
      IfTagPair
        IfStartTag_Start
        ParenthesizedExpression
          LParen
          LogicalAndOperator
            Boolean
            Boolean
          RParen
        TagEnd
        Repeat
          ExpressionStatement
            Variable
        IfEndTag
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
      SharpChar
    IfEndTag
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    cf_if_tag
      parenthesized_expression
        true
      html_text
      cf_if_tag
        parenthesized_expression
          binary_expression
            true
            true
        html_text
      html_text
      hash_single
      html_text
      hash_single
      html_text
      hash_empty
```

### cfml-corpus / cfml.txt / 9: cfscript comments

```
<cfscript>
    // test
    test(); // test

    if ( test EQ test ) {
       test();
    }
    /* test */
</cfscript>
```

**textparser CST** `tp=29 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        Semicolon
      IfStatement
        IfKeyword
        LParen
        CompareOperator
          Variable
          Variable
        RParen
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              PostfixExpressionSuffix
                Variable
                LParen
                RParen
              Semicolon
          RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### cfml-corpus / cfml.txt / 10: cfscript switch

```

<cfscript>
  // test
  switch (test) {
    case "1":
      test();//test

      break;
    default: //test
      test();
  }
</cfscript>

<cfdirectory />
```

**textparser CST** `tp=41 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      SwitchStatement
        SwitchKeyword
        LParen
        Variable
        RParen
        LBrace
        Repeat
          SwitchCase
            CaseKeyword
            DoubleString
            Colon
            Repeat
              ExpressionStatement
                PostfixExpressionSuffix
                  Variable
                  LParen
                  RParen
                Semicolon
              BreakStatement
                BreakKeyword
                Semicolon
          SwitchCase
            DefaultKeyword
            Colon
            Repeat
              ExpressionStatement
                PostfixExpressionSuffix
                  Variable
                  LParen
                  RParen
                Semicolon
        RBrace
    ScriptEndTag
  SelfClosingTag
    StartTag_Start
    TagSelfClose
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_script_tag
      cf_script_content
    cf_selfclose_tag
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
```

### cfml-corpus / cfml.txt / 15: hash-empty attributes, cfif with complex call chain, and cfloop/cfset attribute hashes

```
<a href="##test" style="color: ##fff"></a>
<div test="test" style="color: ##fff;" test="test"> </div>
<cfif compareNoCase(ref_field, "lookup") NEQ 0>
			<cfset data = getService("general").getlookup(companyCode = user.companyCode, lookupType=#lookupType#, addBlankRow="false") />
		</cfif>
<cfloop from="1" to=#arrayLen(test)# index="i"></cfloop>
		<cfset ARGUMENTS.context.utils("setMaxLength","message_text",#maxMsgChars#)>
```

**textparser CST** `tp=154 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  EndTag
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  EndTag
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            Variable
          Repeat
            Sequence
              Separator
              Argument
                DoubleString
        RParen
      Number
    TagEnd
    Repeat
      SetTag
        SetStartTag_Start
        AssignOperator
          Variable
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              PostfixExpressionSuffix
                Variable
                LParen
                ArgumentList
                  Argument
                    DoubleString
                RParen
              ObjectMember
              Variable
            LParen
            ArgumentList
              Argument
                AssignOperator
                  Variable
                  PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
              Repeat
                Sequence
```

**tree-sitter CST** `ts=100 nodes`

```
  program
    element
      start_tag
        tag_name
        tag_attributes
          attribute
            attribute_name
            quoted_attribute_value
        tag_attributes
          attribute
            attribute_name
            quoted_attribute_value
      end_tag
        tag_name
    element
      start_tag
        tag_name
        tag_attributes
          attribute
            attribute_name
            quoted_attribute_value
        tag_attributes
          attribute
            attribute_name
            quoted_attribute_value
        tag_attributes
          attribute
            attribute_name
            quoted_attribute_value
      html_text
      end_tag
        tag_name
    cf_if_tag
      binary_expression
        call_expression
          identifier
          arguments
            identifier
            string
        number
      cf_set_tag
        assignment_expression
          identifier
          call_expression
            member_expression
              call_expression
                identifier
                arguments
                  string
              property_identifier
            arguments
              assignment_expression
                identifier
                member_expression
                  identifier
                  property_identifier
              assignment_expression
                identifier
                hash_expression
                  identifier
              assignment_expression
                identifier
                string
        cf_selfclose_void_tag_end
          self_closing_tag_delimiter
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            cf_attribute_value
              hash_expression
                call_expression
                  identifier
                  arguments
```

### cfml-corpus / cfml.txt / 17: deeply nested cfif chains and complex cfzip/cffile/cfzipparam usage

```
		<cfif true>
			<cfif true>
			</cfif>
      test
      <cfif true>
      </cfif>
    <cfelseif true>
      <cfif true>
			</cfif>
      <cfif true>
      </cfif>
    <cfelseif true>
      <cfif true>
			</cfif>
      <cfif true>
      </cfif>
		</cfif>

    <cfif ARGUMENTS.context.isTarget(id="add-line")
			AND ARGUMENTS.context.getValue("total_amt","0") GT 0.00>
				<cfif StructKeyExists(ARGUMENTS.context.getStatus(type="invalid"),"distlines")>

				<cfelseif distAmtTotal EQ ARGUMENTS.context.getValue("total_amt","0")>

				<cfelseif distAmtTotal GT ARGUMENTS.context.getValue("total_amt","0")>

				</cfif>
			</cfif>
<cfzip action="zip"
					file="#GetTempDirectory()##objName#_Draft.zip"
					overwrite="yes">

					<cfloop index="i" from="1" to="#arrayLen(fileInfo)#">

						<cfloop from="1" to="#ArrayLen(files.data)#" index="j">

							<!--- @CFLintIgnore AVOID_USING_CFFILE_TAG --->
							<cffile action="readBinary"	file="#files.data[j].physicalpath##files.data[j].filename#" variable="filebin" />

							<cfif CompareNoCasE(files.data[j].file_type,"B") EQ 0>
								<cfzipparam
									content="#filebin#"
									entrypath="#fileInfo[i].uid# - #fileInfo[i].name# - #files.data[j].num#.#ListLast(files.data[j].filename,'.')#"
								/>
							<cfelseif CompareNoCasE(files.data[j].file_type,"A") EQ 0>
								<cfzipparam
									content="#filebin#"
									entrypath="#fileInfo[i].uid# - #fileInfo[i].name# - #files.data[j].num#.#ListLast(files.data[j].filename,'.')#"
								/>
							</cfif>

						</cfloop>

					</cfloop>

				</cfzip>
```

**textparser CST** `tp=340 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    Boolean
    TagEnd
    Repeat
      IfTagPair
        IfStartTag_Start
        Boolean
        TagEnd
        IfEndTag
      ExpressionStatement
        Variable
      IfTagPair
        IfStartTag_Start
        Boolean
        TagEnd
        IfEndTag
    Repeat
      Sequence
        ElseIfTag
          ElseIfStartTag_Start
          Boolean
          TagEnd
        Repeat
          IfTagPair
            IfStartTag_Start
            Boolean
            TagEnd
            IfEndTag
          IfTagPair
            IfStartTag_Start
            Boolean
            TagEnd
            IfEndTag
      Sequence
        ElseIfTag
          ElseIfStartTag_Start
          Boolean
          TagEnd
        Repeat
          IfTagPair
            IfStartTag_Start
            Boolean
            TagEnd
            IfEndTag
          IfTagPair
            IfStartTag_Start
            Boolean
            TagEnd
            IfEndTag
    IfEndTag
  IfTagPair
    IfStartTag_Start
    LogicalAndOperator
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            Variable
            ObjectMember
            Variable
          ObjectMember
          Variable
        LParen
        ArgumentList
          Argument
            AssignOperator
              Variable
              DoubleString
        RParen
      CompareOperator
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            ObjectMember
            Variable
          LParen
```

**tree-sitter CST** `ts=276 nodes`

```
  program
    cf_if_tag
      true
      cf_if_tag
        true
      html_text
      cf_if_tag
        true
      cf_if_alt
        cf_elseif_tag
          true
        cf_if_tag
          true
        cf_if_tag
          true
        cf_if_alt
          cf_elseif_tag
            true
          cf_if_tag
            true
          cf_if_tag
            true
    cf_if_tag
      binary_expression
        call_expression
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
            assignment_expression
              identifier
              string
        binary_expression
          call_expression
            member_expression
              member_expression
                identifier
                property_identifier
              property_identifier
            arguments
              string
              string
          number
      cf_if_tag
        call_expression
          identifier
          arguments
            call_expression
              member_expression
                member_expression
                  identifier
                  property_identifier
                property_identifier
              arguments
                assignment_expression
                  identifier
                  string
            string
        cf_if_alt
          cf_elseif_tag
            binary_expression
              identifier
              call_expression
                member_expression
                  member_expression
                    identifier
                    property_identifier
                  property_identifier
                arguments
                  string
                  string
          cf_if_alt
            cf_elseif_tag
              binary_expression
                identifier
                call_expression
                  member_expression
                    member_expression
```

### cfml-corpus / cfml.txt / 18: cfxml

```
<cfxml variable="theXml">
			<cfoutput>
			<entity>
				<field><![CDATA[#result.field#]]></field>
			</entity>
			</cfoutput>
		</cfxml>
```

**textparser CST** `tp=41 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        TagEnd
      StartTag
        StartTag_Start
        TagEnd
      CompareOperator
      LogicalNotOperator
      LBracket
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LBracket
          SharpExpression
            SharpExpression_Start
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            SharpExpression_Start
          RBracket
      RBracket
      CompareOperator
      EndTag
      EndTag
    OutputEndTag
  EndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_xml_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_xml_content
```

### cfml-corpus / cfml.txt / 21: var vs VARIABLES

```
<cfset VARIABLES.test = StructNew()>
<cfset var test = StructNew()>
<cfset VAR test = StructNew()>
```

**textparser CST** `tp=41 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        LParen
        RParen
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
    TagEnd
  SetTag
    SetStartTag_Start
    VariableDeclarationStatement
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
    TagEnd
```

**tree-sitter CST** `ts=26 nodes`

```
  program
    cf_set_tag
      assignment_expression
        member_expression
          identifier
          property_identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
    cf_set_tag
      cf_var
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
      cf_selfclose_void_tag_end
```

### cfml-corpus / cfml.txt / 23: cfzip3

```
<cffunction name="validateAndUnzipFile" access="public" output="false" returntype="any">
		<cfargument name="context" type="struct" required="true" />

		<cfif result.bZipError IS true>

			<!--- Don't return any files --->
			<cfset arrFiles = arrayNew(1)>

		<cfelse>

			<cfzip action="unzip" destination="#getTempDirectory()#" file="#ARGUMENTS.zipFilePath#" overwrite="true" />

    </cfif>

	</cffunction>
```

**textparser CST** `tp=97 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  SelfClosingTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        RequiredKeyword
        Sequence
          AssignOperator
          DoubleString
    TagSelfClose
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      Boolean
    TagEnd
    Repeat
      SetTag
        SetStartTag_Start
        AssignOperator
          Variable
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                Number
            RParen
        TagEnd
    Sequence
      ElseTag
        ElseStartTag_Start
        TagEnd
      Repeat
        SelfClosingTag
          StartTag_Start
          Repeat
            TagAttribute
              Variable
              Sequence
                AssignOperator
                DoubleString
            TagAttribute
```

**tree-sitter CST** `ts=67 nodes`

```
  program
    cf_function_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_tag
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_selfclose_void_tag_end
          self_closing_tag_delimiter
      cf_if_tag
        binary_expression
          member_expression
            identifier
            property_identifier
          true
        cf_comment
        cf_set_tag
          assignment_expression
            identifier
            call_expression
              identifier
              arguments
                number
          cf_selfclose_void_tag_end
        cf_if_alt
          cf_else_tag
          cf_tag
            cf_start_tag_with_selfclose
              cf_tag_name
              cf_tag_attributes
                cf_attribute
                  cf_attribute_name
                  quoted_cf_attribute_value
              cf_tag_attributes
                cf_attribute
                  cf_attribute_name
                  quoted_cf_attribute_value
                  #
                  #
              cf_tag_attributes
                cf_attribute
                  cf_attribute_name
                  quoted_cf_attribute_value
                  #
                  .
                  #
              cf_tag_attributes
                cf_attribute
                  cf_attribute_name
                  quoted_cf_attribute_value
```

### cfml-corpus / cfml.txt / 24: cfset string-plus-hash expression and cfswitch inside cfquery

```
<cfset downloadUrl = "https:"&#ARGUMENTS.download_url# />

<cfquery name="Test">
  <cfswitch expression="#tmp_budget_no#">
    <cfcase value="1">
    </cfcase>
  </cfswitch>
</cfquery>
```

**textparser CST** `tp=46 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      ConcatOperator
        DoubleString
        SharpExpression
          SharpExpression_Start
          PostfixExpressionSuffix
            Variable
            ObjectMember
            Variable
          SharpExpression_Start
    TagSelfClose
  QueryTagPair
    QueryStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
      StartTag
        StartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
      EndTag
      EndTag
    QueryEndTag
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        binary_expression
          string
          hash_expression
            member_expression
              identifier
              property_identifier
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
    cf_query_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_query_content
```

### cfml-corpus / cfml.txt / 26: cftransaction2

```
<cffunction name="Test">

  <cftransaction isolation="read_committed" action="begin">
  <cftransaction action="commit">
  </cftransaction>

    <cftransaction isolation="read_committed" action="begin">
  <cftransaction action="commit" />
  </cftransaction>

  <cfreturn result />

  </cffunction>
```

**textparser CST** `tp=65 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  EndTag
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  SelfClosingTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagSelfClose
  EndTag
  SelfClosingTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
    TagSelfClose
  EndTag
```

**tree-sitter CST** `ts=50 nodes`

```
  program
    cf_function_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_tag
        cf_start_tag
          cf_tag_name
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
        cf_tag
          cf_start_tag
            cf_tag_name
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
          cf_end_tag
            cf_tag_name
        cf_tag
          cf_start_tag
            cf_tag_name
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
          cf_tag
            cf_start_tag_with_selfclose
              cf_tag_name
              cf_tag_attributes
                cf_attribute
                  cf_attribute_name
                  quoted_cf_attribute_value
          cf_end_tag
            cf_tag_name
        cf_return_tag
          identifier
          cf_selfclose_void_tag_end
            self_closing_tag_delimiter
        implicit_cf_end_tag
```

### cfml-corpus / cfml.txt / 27: cfoutput boolean expression with JS-style operators

```
<cfoutput>
  #(x > 0 && y <= 10) || z == 5#
</cfoutput>
```

**textparser CST** `tp=23 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        LogicalOrOperator
          ParenthesizedExpression
            LParen
            LogicalAndOperator
              CompareOperator
                Variable
                Number
              CompareOperator
                Variable
                Number
            RParen
          CompareOperator
            Variable
            Number
        SharpExpression_Start
    OutputEndTag
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    cf_output_tag
      html_text
      hash_expression
        binary_expression
          parenthesized_expression
            binary_expression
              binary_expression
                identifier
                number
              binary_expression
                identifier
                number
          binary_expression
            identifier
            number
```

### cfml-corpus / cfml.txt / 30: component ( cfscript )

```
component {
  private function sign( any message, any key, any algorithm ) {
        var tmpkey = "";
        if ( left( ARGUMENTS.algorithm, 1 ) == 'H' ) {
            var sig = binaryDecode(
                hmac(
                    ARGUMENTS.message,
                    ARGUMENTS.key,
                    algorithmMap[ ARGUMENTS.algorithm ],
                    'utf-8'
                ),
                'hex'
            );
        } else {
            if ( isSimpleValue( ARGUMENTS.key ) ) {
                tmpkey = encodingUtils.parsePEMEncodedKey( ARGUMENTS.key );
            } else if ( isStruct( ARGUMENTS.key ) ) {
                tmpkey = encodingUtils.parseJWK( ARGUMENTS.key );
            } else {
                tmpkey = ARGUMENTS.key;
            }

            var jssInstance = variables.jss.getInstance( algorithmMap[ ARGUMENTS.algorithm ] );
            jssInstance.initSign( tmpkey );
            jssInstance.update( charsetDecode( ARGUMENTS.message, 'utf-8' ) );
            var sig = jssInstance.sign();
            if ( variables.javaVersion < 11 && left( ARGUMENTS.algorithm, 1 ) == 'E' ) {
                sig = encodingUtils.convertDERtoP1363( sig, ARGUMENTS.algorithm );
            }
        }
        return sig;
    }
}
```

**textparser CST** `tp=348 nodes`

```
ComponentDeclaration
  ComponentKeyword
  LBrace
  Repeat
    FunctionDeclaration
      Repeat
        PrivateKeyword
      FunctionKeyword
      Variable
      LParen
      ParameterList
        Parameter
          Variable
          Variable
        Repeat
          Sequence
            Separator
            Parameter
              Variable
              Variable
          Sequence
            Separator
            Parameter
              Variable
              Variable
      RParen
      BlockStatement
        LBrace
        Repeat
          VariableDeclarationStatement
            VarKeyword
            VariableDeclaratorList
              VariableDeclarator
                Variable
                Sequence
                  AssignOperator
                  DoubleString
            Semicolon
          IfStatement
            IfKeyword
            LParen
            CompareOperator
              PostfixExpressionSuffix
                Variable
                LParen
                ArgumentList
                  Argument
                    PostfixExpressionSuffix
                      Variable
                      ObjectMember
                      Variable
                  Repeat
                    Sequence
                      Separator
                      Argument
                        Number
                RParen
              SingleString
            RParen
            BlockStatement
              LBrace
              Repeat
                VariableDeclarationStatement
                  VarKeyword
                  VariableDeclaratorList
                    VariableDeclarator
                      Variable
                      Sequence
                        AssignOperator
                        PostfixExpressionSuffix
                          Variable
                          LParen
                          ArgumentList
                            Argument
                              PostfixExpressionSuffix
                                Variable
                                LParen
                                ArgumentList
                                  Argument
                                    PostfixExpressionSuffix
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    component_file
      cf_component_content
```

### cfml-corpus / cfml.txt / 31: abstract component ( cfscript )

```
abstract component {
  public function test() { }
}
```

**textparser CST** `tp=17 nodes`

```
ComponentDeclaration
  Repeat
    AbstractKeyword
  ComponentKeyword
  LBrace
  Repeat
    FunctionDeclaration
      Repeat
        PublicKeyword
      FunctionKeyword
      Variable
      LParen
      RParen
      BlockStatement
        LBrace
        RBrace
  RBrace
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    component_file
      cf_component_content
```

### cfml-corpus / cfml.txt / 32: static component ( cfscript )

```
static component {
  public function test() { }
}
```

**textparser CST** `tp=17 nodes`

```
ComponentDeclaration
  Repeat
    StaticKeyword
  ComponentKeyword
  LBrace
  Repeat
    FunctionDeclaration
      Repeat
        PublicKeyword
      FunctionKeyword
      Variable
      LParen
      RParen
      BlockStatement
        LBrace
        RBrace
  RBrace
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    component_file
      cf_component_content
```

### cfml-corpus / cfml.txt / 33: final component ( cfscript )

```
final component {
  public function test() { }
}
```

**textparser CST** `tp=17 nodes`

```
ComponentDeclaration
  Repeat
    FinalKeyword
  ComponentKeyword
  LBrace
  Repeat
    FunctionDeclaration
      Repeat
        PublicKeyword
      FunctionKeyword
      Variable
      LParen
      RParen
      BlockStatement
        LBrace
        RBrace
  RBrace
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    component_file
      cf_component_content
```

### cfml-corpus / cfml.txt / 34: FINAL component, case-insensitive ( cfscript )

```
FINAL component {
  public function test() { }
}
```

**textparser CST** `tp=17 nodes`

```
ComponentDeclaration
  Repeat
    FinalKeyword
  ComponentKeyword
  LBrace
  Repeat
    FunctionDeclaration
      Repeat
        PublicKeyword
      FunctionKeyword
      Variable
      LParen
      RParen
      BlockStatement
        LBrace
        RBrace
  RBrace
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    component_file
      cf_component_content
```

### cfml-corpus / cfml.txt / 35: a modifier word that is not a component file stays text

```
final xyz { }
```

**textparser CST** `tp=6 nodes`

```
Template
  Statement
    FinalKeyword
    Variable
    LBrace
  RBrace
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    html_text
    text
```

### cfml-corpus / cfml.txt / 38: obscure test 1

```

<cfif ARGUMENTS.step GT 1>

	<cfset tempPreviousRoute = URLDecode(getToken(previousRoute,ARGUMENTS.step, '|')) />
	<cfset tempHash = URLDecode(ListRest(tempPreviousRoute,'##')) />

	<cfif Len(Trim(tempPreviousRoute)) GT 0>

		<cfset result = "#ListFirst(tempPreviousRoute,'##')#" />

		<cfif ListLen(tempPreviousRoute,'|') GT ARGUMENTS.step>

			<cfset tempNewPreviousRoute = "" />
			<cfset tempNewPreviousCriteria = "" />

			<cfloop from="#ARGUMENTS.step#" to="#ListLen(tempPreviousRoute,'|')#" index="j">
				<cfset tempNewPreviousRoute = ListAppend(getToken(previousRoute, j, '|'),'|') />
				<cfset tempNewPreviousCriteria = ListAppend(getToken(previousCriteria, j, '|'),'|') />
			</cfloop>

			<cfif FindNoCase(".cfm",ListFirst(tempPreviousRoute,'##')) GT 0
			AND findNoCase("?", ListFirst(tempPreviousRoute,'##')) LTE 0>
				<cfset result = "#result#?previousRoute=#ReplaceList(tempNewPreviousRoute,'&,##,=,?,|','%26,%23,%3D,%3F,%7C')#&previousCriteria=#ReplaceList(tempNewPreviousCriteria,'&,##,=,|','%26,%23,%3D,%7C')#" />
			<cfelse>
				<cfset result = "#result#&previousRoute=#ReplaceList(tempNewPreviousRoute,'&,##,=,?,|','%26,%23,%3D,%3F,%7C')#&previousCriteria=#ReplaceList(tempNewPreviousCriteria,'&,##,=,|','%26,%23,%3D,%7C')#" />
			</cfif>

		</cfif>

	</cfif>

<cfelseif ListLen(previousRoute,"|") GT 1>

	<cfset tempPreviousRoute = URLDecode(ListFirst(previousRoute,'|')) />
	<cfset tempHash = URLDecode(ListRest(tempPreviousRoute,'##')) />
	<cfif FindNoCase(".cfm",ListFirst(tempPreviousRoute,'##')) GT 0
	AND findNoCase("?", ListFirst(tempPreviousRoute,'##')) LTE 0>
		<cfset result = "#ListFirst(tempPreviousRoute,'##')#?previousRoute=#ReplaceList(ListRest(previousRoute,'|'),'&,##,=,?,|','%26,%23,%3D,%3F,%7C')#&previousCriteria=#ReplaceList(ListRest(previousCriteria,'|'),'&,##,=,|','%26,%23,%3D,%7C')#" />
	<cfelse>
		<cfset result = "#ListFirst(tempPreviousRoute,'##')#&previousRoute=#ReplaceList(ListRest(previousRoute,'|'),'&,##,=,?,|','%26,%23,%3D,%3F,%7C')#&previousCriteria=#ReplaceList(ListRest(previousCriteria,'|'),'&,##,=,|','%26,%23,%3D,%7C')#" />
	</cfif>

<cfelse>

	<cfset previousRoute = URLDecode(previousRoute) />
	<cfif FindNoCase(".cfm",previousRoute) GT 0
	AND findNoCase("?", previousRoute) LTE 0>
		<cfset previousRoute = previousRoute & "?" />
	</cfif>
	<cfset result = ListFirst(previousRoute,'##') />
	<cfset tempHash = ListRest(previousRoute,'##') />

</cfif>

<container:lockaction mode="set" batch_ID="#URL.batch_ID#" returnVariable="recordLocked">
	<cfloop from="1" to="#ArrayLen(arrExisting)#" index="j">
		<container:lockactionitem table_ID="arrExisting[j].table_id" record_ID="#arrExisting[j].record_id#">
	</cfloop>
</container:lockaction>

<div>
	<cfloop>
		<span>
	</cfloop>
</div>
```

**textparser CST** `tp=582 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      Number
    TagEnd
    Repeat
      SetTag
        SetStartTag_Start
        AssignOperator
          Variable
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                PostfixExpressionSuffix
                  Variable
                  LParen
                  ArgumentList
                    Argument
                      Variable
                    Repeat
                      Sequence
                        Separator
                        Argument
                          PostfixExpressionSuffix
                            Variable
                            ObjectMember
                            Variable
                      Sequence
                        Separator
                        Argument
                          SingleString
                  RParen
            RParen
        TagSelfClose
      SetTag
        SetStartTag_Start
        AssignOperator
          Variable
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                PostfixExpressionSuffix
                  Variable
                  LParen
                  ArgumentList
                    Argument
                      Variable
                    Repeat
                      Sequence
                        Separator
                        Argument
                          SingleString
                  RParen
            RParen
        TagSelfClose
      IfTagPair
        IfStartTag_Start
        CompareOperator
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                PostfixExpressionSuffix
                  Variable
                  LParen
                  ArgumentList
                    Argument
                      Variable
                  RParen
            RParen
```

**tree-sitter CST** `ts=489 nodes`

```
  program
    cf_if_tag
      binary_expression
        member_expression
          identifier
          property_identifier
        number
      cf_set_tag
        assignment_expression
          identifier
          call_expression
            identifier
            arguments
              call_expression
                identifier
                arguments
                  identifier
                  member_expression
                    identifier
                    property_identifier
                  string
                    string_fragment
        cf_selfclose_void_tag_end
          self_closing_tag_delimiter
      cf_set_tag
        assignment_expression
          identifier
          call_expression
            identifier
            arguments
              call_expression
                identifier
                arguments
                  identifier
                  string
                    hash_empty
        cf_selfclose_void_tag_end
          self_closing_tag_delimiter
      cf_if_tag
        binary_expression
          call_expression
            identifier
            arguments
              call_expression
                identifier
                arguments
                  identifier
          number
        cf_set_tag
          assignment_expression
            identifier
            string
            #
              ,
              '
              #
              #
              '
            #
          cf_selfclose_void_tag_end
            self_closing_tag_delimiter
        cf_if_tag
          binary_expression
            call_expression
              identifier
              arguments
                identifier
                string
                  string_fragment
            member_expression
              identifier
              property_identifier
          cf_set_tag
            assignment_expression
              identifier
              string
            cf_selfclose_void_tag_end
              self_closing_tag_delimiter
          cf_set_tag
            assignment_expression
```

### cfml-corpus / cfml.txt / 41: cfobject void tag

```
<cfobject component="models.Base" name="obj">
```

**textparser CST** `tp=15 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
```

### cfml-corpus / cfml.txt / 43: ordered struct literal in cfscript block

```
<cfscript>
x = [:];
</cfscript>
```

**textparser CST** `tp=14 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          StructLiteral
            LBracket
            Colon
            RBracket
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### cfml-corpus / cfml.txt / 45: doubled-quote escape in tag attribute values

```
<cfparam name="dq" default="p ""q"" r">
<cfparam name="sq" default='x ''y'' z'>
<cfparam name="ed" default="">
<cfparam name="es" default=''>
```

**textparser CST** `tp=57 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          SingleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          SingleString
    TagEnd
```

**tree-sitter CST** `ts=36 nodes`

```
  program
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
          attribute_value
          attribute_value
          attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
```

### cfml-corpus / cfoutput_script.txt / 0: script element inside cfoutput enables hash expressions

```
<cfoutput><script>var x = #myVar#;</script></cfoutput>
```

**textparser CST** `tp=22 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        TagEnd
      VariableDeclarationStatement
        VarKeyword
        VariableDeclaratorList
          VariableDeclarator
            Variable
            Sequence
              AssignOperator
              SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
        Semicolon
      EndTag
    OutputEndTag
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    cf_output_tag
      script_element
        start_tag
          tag_name
        script_text
        hash_expression
          identifier
        script_text
        end_tag
          tag_name
```

### cfml-corpus / cfoutput_script.txt / 1: script element without cfoutput treats hash as raw text

```
<script>var x = #myVar#;</script>
```

**textparser CST** `tp=17 nodes`

```
Template
  StartTag
    StartTag_Start
    TagEnd
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          SharpExpression
            SharpExpression_Start
            Variable
            SharpExpression_Start
    Semicolon
  EndTag
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    script_element
      start_tag
        tag_name
      script_text
      end_tag
        tag_name
```

### cfml-corpus / common.txt / 1: common: cfoutput with query and group

```

<cfoutput query="q" group="category">#category#<cfoutput>#q.name#</cfoutput></cfoutput>
```

**textparser CST** `tp=33 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
      OutputTagPair
        OutputStartTag_Start
        TagEnd
        Repeat
          SharpExpression
            SharpExpression_Start
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            SharpExpression_Start
        OutputEndTag
    OutputEndTag
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    cf_output_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      hash_expression
        identifier
      cf_output_tag
        hash_expression
          member_expression
            identifier
            property_identifier
```

### cfml-corpus / common.txt / 10: common: cfstoredproc with procparam and procresult

```

<cfstoredproc procedure="sp_x" datasource="ds"><cfprocparam type="in" cfsqltype="cf_sql_integer" value="1"><cfprocresult name="r"></cfstoredproc>
```

**textparser CST** `tp=44 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  EndTag
```

**tree-sitter CST** `ts=30 nodes`

```
  program
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
      cf_selfclose_tag
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_selfclose_void_tag_end
      cf_selfclose_tag
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_selfclose_void_tag_end
      cf_end_tag
        cf_tag_name
```

### cfml-corpus / common.txt / 12: common: query of queries with result and maxrows

```

<cfquery name="sub" dbtype="query" result="meta" maxrows="10">SELECT id FROM q WHERE id > 1</cfquery>
```

**textparser CST** `tp=41 nodes`

```
Template
  QueryTagPair
    QueryStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        CompareOperator
          Variable
          Number
    QueryEndTag
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    cf_query_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_query_content
```

### cfml-corpus / common.txt / 14: common: cfmodule, cfinclude and cfabort

```

<cfmodule template="/t.cfm" attr="1"><cfinclude template="header.cfm"><cfif x><cfabort></cfif>
```

**textparser CST** `tp=33 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  IfTagPair
    IfStartTag_Start
    Variable
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        TagEnd
    IfEndTag
```

**tree-sitter CST** `ts=22 nodes`

```
  program
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
      cf_selfclose_tag
        cf_attribute
          cf_attribute_name
          quoted_cf_attribute_value
        cf_selfclose_void_tag_end
      cf_if_tag
        identifier
        cf_selfclose_tag
          cf_selfclose_void_tag_end
      implicit_cf_end_tag
```

### cfml-corpus / common.txt / 15: common: cfheader, cfcontent and cfcookie

```

<cfheader name="X" value="1"><cfcontent type="application/json"><cfcookie name="c" value="v" expires="never">
```

**textparser CST** `tp=43 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
```

**tree-sitter CST** `ts=25 nodes`

```
  program
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
```

### cfml-corpus / common.txt / 21: common: cfspreadsheet, cfimage and cffeed

```

<cfspreadsheet action="write" filename="f.xls" query="q"><cfimage action="resize" source="a.png" width="10"><cffeed action="read" source="u" query="r">
```

**textparser CST** `tp=58 nodes`

```
Template
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
  StartTag
    StartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
```

**tree-sitter CST** `ts=39 nodes`

```
  program
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_selfclose_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_selfclose_void_tag_end
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
      implicit_cf_end_tag
```

### cfml-corpus / common.txt / 25: common: cfassociate and cfexit in a custom tag

```

<cfif thisTag.executionMode EQ "start"><cfassociate basetag="cf_parent" datacollection="items"><cfexit method="exittemplate"></cfif>
```

**textparser CST** `tp=35 nodes`

```
Template
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      DoubleString
    TagEnd
    Repeat
      StartTag
        StartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
      StartTag
        StartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
    IfEndTag
```

**tree-sitter CST** `ts=24 nodes`

```
  program
    cf_if_tag
      binary_expression
        member_expression
          identifier
          property_identifier
        string
      cf_tag
        cf_start_tag
          cf_tag_name
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
        cf_selfclose_tag
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
          cf_selfclose_void_tag_end
        implicit_cf_end_tag
```

### cfml-corpus / common.txt / 27: common: deeply nested tag chains

```

<cfoutput><cfif a><cfloop from="1" to="3" index="i"><cfif b><cfswitch expression="#i#"><cfcase value="1"><cfif c>#i#</cfif></cfcase></cfswitch></cfif></cfloop></cfif></cfoutput>
```

**textparser CST** `tp=69 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      IfTagPair
        IfStartTag_Start
        Variable
        TagEnd
        Repeat
          LoopTagPair
            LoopStartTag_Start
            Repeat
              TagAttribute
                Variable
                Sequence
                  AssignOperator
                  DoubleString
              TagAttribute
                Variable
                Sequence
                  AssignOperator
                  DoubleString
              TagAttribute
                Variable
                Sequence
                  AssignOperator
                  DoubleString
            TagEnd
            Repeat
              IfTagPair
                IfStartTag_Start
                Variable
                TagEnd
                Repeat
                  StartTag
                    StartTag_Start
                    Repeat
                      TagAttribute
                        Variable
                        Sequence
                          AssignOperator
                          DoubleString
                    TagEnd
                  StartTag
                    StartTag_Start
                    Repeat
                      TagAttribute
                        Variable
                        Sequence
                          AssignOperator
                          DoubleString
                    TagEnd
                  IfTagPair
                    IfStartTag_Start
                    Variable
                    TagEnd
                    Repeat
                      SharpExpression
                        SharpExpression_Start
                        Variable
                        SharpExpression_Start
                    IfEndTag
                  EndTag
                  EndTag
                IfEndTag
            LoopEndTag
        IfEndTag
    OutputEndTag
```

**tree-sitter CST** `ts=47 nodes`

```
  program
    cf_output_tag
      cf_if_tag
        identifier
        cf_tag
          cf_start_tag
            cf_tag_name
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
            cf_tag_attributes
              cf_attribute
                cf_attribute_name
                quoted_cf_attribute_value
          cf_if_tag
            identifier
            cf_tag
              cf_start_tag
                cf_tag_name
                cf_tag_attributes
                  cf_attribute
                    cf_attribute_name
                    quoted_cf_attribute_value
                    #
                    #
              cf_tag
                cf_start_tag
                  cf_tag_name
                  cf_tag_attributes
                    cf_attribute
                      cf_attribute_name
                      quoted_cf_attribute_value
                cf_if_tag
                  identifier
                  hash_expression
                    identifier
                cf_end_tag
                  cf_tag_name
              cf_end_tag
                cf_tag_name
          cf_end_tag
            cf_tag_name
```

### cfml-corpus / common.txt / 29: common: bare angle brackets in template text

```

<p>a > b</p>
<p>5 < 6</p>
<cfoutput>#a# > #b#</cfoutput>
<em>Hit Ratio:</em> #ratio#%  ==>
```

**textparser CST** `tp=46 nodes`

```
Template
  StartTag
    StartTag_Start
    TagEnd
  ExpressionStatement
    CompareOperator
      Variable
      Variable
  EndTag
  StartTag
    StartTag_Start
    TagEnd
  Number
  CompareOperator
  Number
  EndTag
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
      CompareOperator
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
    OutputEndTag
  StartTag
    StartTag_Start
    TagEnd
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Colon
  EndTag
  SharpExpression
    SharpExpression_Start
    Variable
    SharpExpression_Start
  ModOperator
  CompareOperator
  CompareOperator
```

**tree-sitter CST** `ts=30 nodes`

```
  program
    element
      start_tag
        tag_name
      html_text
      end_tag
        tag_name
    element
      start_tag
        tag_name
      html_text
      end_tag
        tag_name
    cf_output_tag
      hash_expression
        identifier
      html_text
      hash_expression
        identifier
    element
      start_tag
        tag_name
      html_text
      end_tag
        tag_name
    html_text
    hash_single
    html_text
    hash_single
    html_text
```

### cfml-corpus / real_world.txt / 0: real world: dynamic closing tag name (Taffy ArrayToXML)

```

<cfoutput><#arguments.node#>#arguments.value#</#arguments.node#></cfoutput>
```

**textparser CST** `tp=32 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      CompareOperator
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        SharpExpression_Start
      CompareOperator
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        SharpExpression_Start
      CompareOperator
      MulOperator
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        SharpExpression_Start
      CompareOperator
    OutputEndTag
```

**tree-sitter CST** `ts=19 nodes`

```
  program
    cf_output_tag
      element
        start_tag
          tag_name
            hash_expression
              member_expression
                identifier
                property_identifier
        hash_expression
          member_expression
            identifier
            property_identifier
        end_tag
          tag_name
            hash_expression
              member_expression
                identifier
                property_identifier
```

### cfml-corpus / real_world.txt / 2: real world: param statement inside cfscript (Lucee LDEV4374)

```

<cfscript>
	param name="FORM.scene" default="";
</cfscript>
```

**textparser CST** `tp=20 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ParamStatement
        ParamKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            DefaultKeyword
            Sequence
              AssignOperator
              DoubleString
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### cfml-corpus / real_world.txt / 3: real world: cfcontinue inside cfloop inside cfoutput (TestBox mintext.cfm)

```

<cfoutput>#getHeaderBanner( testbox )#
<cfloop array="#variables.bundleStats#" index="thisBundle">
<cfif len( url.testBundles )>
<cfcontinue>
</cfif>
#thisBundle.path#
</cfloop>
</cfoutput>
```

**textparser CST** `tp=57 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
          RParen
        SharpExpression_Start
      LoopTagPair
        LoopStartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
        Repeat
          IfTagPair
            IfStartTag_Start
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
              RParen
            TagEnd
            Repeat
              StartTag
                StartTag_Start
                TagEnd
            IfEndTag
          SharpExpression
            SharpExpression_Start
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            SharpExpression_Start
        LoopEndTag
    OutputEndTag
```

**tree-sitter CST** `ts=37 nodes`

```
  program
    cf_output_tag
      hash_expression
        call_expression
          identifier
          arguments
            identifier
      cf_tag
        cf_start_tag
          cf_tag_name
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
              #
              .
              #
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
        cf_if_tag
          call_expression
            identifier
            arguments
              member_expression
                identifier
                property_identifier
          cf_selfclose_tag
            cf_selfclose_void_tag_end
        html_text
        hash_expression
          member_expression
            identifier
            property_identifier
        cf_end_tag
          cf_tag_name
```

### cfml-corpus / real_world.txt / 4: real world: function call in a cfif condition (Lucee admin templates)

```

<cfloop array="#driver.getCustomFields()#" index="field">
<cfif isInstanceOf( field, "Group" )>ok</cfif>
</cfloop>
```

**textparser CST** `tp=36 nodes`

```
Template
  LoopTagPair
    LoopStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      IfTagPair
        IfStartTag_Start
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
            Repeat
              Sequence
                Separator
                Argument
                  DoubleString
          RParen
        TagEnd
        Repeat
          ExpressionStatement
            Variable
        IfEndTag
    LoopEndTag
```

**tree-sitter CST** `ts=24 nodes`

```
  program
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
            #
            .
            #
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
      cf_if_tag
        call_expression
          identifier
          arguments
            identifier
            string
        html_text
      cf_end_tag
        cf_tag_name
```

### script-corpus / case_insensitivity.txt / 0: statement keywords are case-insensitive

```
IF (x) { y = 1; } ELSE { y = 2; }
WHILE (x) { break; }
DO { x = 1; } WHILE (x);
FOR (i = 1; i < 2; i++) { CONTINUE; }
TRY { x = 1; } CATCH (any e) { } FINALLY { }
SWITCH (x) { CASE 1: BREAK; DEFAULT: BREAK; }
RETURN 1;
```

**textparser CST** `tp=128 nodes`

```
Repeat
  IfStatement
    IfKeyword
    LParen
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          AssignOperator
            Variable
            Number
          Semicolon
      RBrace
    Sequence
      ElseKeyword
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            AssignOperator
              Variable
              Number
            Semicolon
        RBrace
  WhileStatement
    WhileKeyword
    LParen
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        BreakStatement
          BreakKeyword
          Semicolon
      RBrace
  DoWhileStatement
    DoKeyword
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          AssignOperator
            Variable
            Number
          Semicolon
      RBrace
    WhileKeyword
    LParen
    Variable
    RParen
    Semicolon
  StandardForStatement
    ForKeyword
    LParen
    AssignOperator
      Variable
      Number
    Semicolon
    CompareOperator
      Variable
      Number
    Semicolon
    IncDecOperator
      Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ContinueStatement
          ContinueKeyword
          Semicolon
      RBrace
  TryCatchStatement
    TryKeyword
    BlockStatement
      LBrace
      Repeat
```

**tree-sitter CST** `ts=62 nodes`

```
  program
    if_statement
      parenthesized_expression
        identifier
      statement_block
        expression_statement
          assignment_expression
            identifier
            number
      else_clause
        statement_block
          expression_statement
            assignment_expression
              identifier
              number
    while_statement
      parenthesized_expression
        identifier
      statement_block
        break_statement
    do_statement
      statement_block
        expression_statement
          assignment_expression
            identifier
            number
      parenthesized_expression
        identifier
    for_statement
      assignment_expression
        identifier
        number
      binary_expression
        identifier
        number
      update_expression
        identifier
      statement_block
        continue_statement
    try_statement
      statement_block
        expression_statement
          assignment_expression
            identifier
            number
      catch_clause
        catch_type
        identifier
        statement_block
      finally_clause
        statement_block
    switch_statement
      parenthesized_expression
        identifier
      switch_body
        switch_case
          number
          break_statement
        switch_default
          break_statement
    return_statement
      number
```

### script-corpus / case_insensitivity.txt / 1: mixed-case statement keywords

```
If (x) { Return 1; }
While (x) { Break; }
Try { } Catch (any e) { }
Var y = New Foo();
Throw "boom";
```

**textparser CST** `tp=59 nodes`

```
Repeat
  IfStatement
    IfKeyword
    LParen
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ReturnStatement
          ReturnKeyword
          Number
          Semicolon
      RBrace
  WhileStatement
    WhileKeyword
    LParen
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        BreakStatement
          BreakKeyword
          Semicolon
      RBrace
  TryCatchStatement
    TryKeyword
    BlockStatement
      LBrace
      RBrace
    Repeat
      CatchClause
        CatchKeyword
        LParen
        Variable
        Variable
        RParen
        BlockStatement
          LBrace
          RBrace
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          NewExpression
            NewKeyword
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
    Semicolon
  ThrowStatement
    ThrowKeyword
    DoubleString
    Semicolon
```

**tree-sitter CST** `ts=26 nodes`

```
  program
    if_statement
      parenthesized_expression
        identifier
      statement_block
        return_statement
          number
    while_statement
      parenthesized_expression
        identifier
      statement_block
        break_statement
    try_statement
      statement_block
      catch_clause
        catch_type
        identifier
        statement_block
    variable_declaration
      variable_declarator
        identifier
        new_expression
          identifier
          arguments
    throw_statement
      string
```

### script-corpus / case_insensitivity.txt / 2: access modifiers are case-insensitive

```
PUBLIC function a() {}
Private function b() {}
PACKAGE function c() {}
Remote function d() {}
PRIVATE STRING function e() {}
public STATIC function f() {}
ABSTRACT function g();
FINAL function h() {}
```

**textparser CST** `tp=80 nodes`

```
Repeat
  FunctionDeclaration
    Repeat
      PublicKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    Repeat
      PrivateKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    Repeat
      PackageKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    Repeat
      RemoteKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    Repeat
      PrivateKeyword
    Variable
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    Repeat
      PublicKeyword
      StaticKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
  Statement
    AbstractKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
  Semicolon
  FunctionDeclaration
    Repeat
      FinalKeyword
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
```

**tree-sitter CST** `ts=42 nodes`

```
  program
    function_declaration
      access_type
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      identifier
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      access_type
      identifier
      formal_parameters
      statement_block
    function_declaration
      access_type
      identifier
      formal_parameters
    function_declaration
      access_type
      identifier
      formal_parameters
      statement_block
```

### script-corpus / case_insensitivity.txt / 5: throw with named arguments

```
throw(testabc:"saml", message:"Error");
throw(type="x", message="y");
throw("plain");
throw e;
```

**textparser CST** `tp=38 nodes`

```
Repeat
  ThrowStatement
    ThrowKeyword
  Statement
    LParen
    Variable
    Colon
    DoubleString
    Separator
    Variable
    Colon
    DoubleString
    RParen
  Semicolon
  ThrowStatement
    ThrowKeyword
  Statement
    LParen
    Variable
    AssignOperator
    DoubleString
    Separator
    Variable
    AssignOperator
    DoubleString
    RParen
  Semicolon
  ThrowStatement
    ThrowKeyword
    ParenthesizedExpression
      LParen
      DoubleString
      RParen
    Semicolon
  ThrowStatement
    ThrowKeyword
    Variable
    Semicolon
```

**tree-sitter CST** `ts=22 nodes`

```
  program
    throw_statement
      arguments
        pair
          property_identifier
          string
        pair
          property_identifier
          string
    throw_statement
      arguments
        assignment_expression
          identifier
          string
        assignment_expression
          identifier
          string
    throw_statement
      arguments
        string
    throw_statement
      identifier
```

### script-corpus / case_insensitivity.txt / 6: named arguments and pairs do not collide with switch case

```
f(a:1, b:2);
x = [a:1, b:2];
switch (v) { case 1: f(a:1); break; default: break; }
```

**textparser CST** `tp=65 nodes`

```
Repeat
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    Colon
    Number
    Separator
    Variable
    Colon
    Number
    RParen
  Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      StructLiteral
        LBracket
        StructMemberList
          StructMember
            Variable
            Colon
            Number
          Repeat
            Sequence
              Separator
              StructMember
                Variable
                Colon
                Number
        RBracket
    Semicolon
  SwitchStatement
    SwitchKeyword
    LParen
    Variable
    RParen
    LBrace
    Repeat
      SwitchCase
        CaseKeyword
        Number
        Colon
        Repeat
          ExpressionStatement
            Variable
          Statement
            LParen
            Variable
            Colon
            Number
            RParen
          Semicolon
          BreakStatement
            BreakKeyword
            Semicolon
      SwitchCase
        DefaultKeyword
        Colon
        Repeat
          BreakStatement
            BreakKeyword
            Semicolon
    RBrace
```

**tree-sitter CST** `ts=37 nodes`

```
  program
    expression_statement
      call_expression
        identifier
        arguments
          pair
            property_identifier
            number
          pair
            property_identifier
            number
    expression_statement
      assignment_expression
        identifier
        array
          pair
            property_identifier
            number
          pair
            property_identifier
            number
    switch_statement
      parenthesized_expression
        identifier
      switch_body
        switch_case
          number
          expression_statement
            call_expression
              identifier
              arguments
                pair
                  property_identifier
                  number
          break_statement
        switch_default
          break_statement
```

### script-corpus / case_insensitivity.txt / 11: import and new keep their statement meanings

```
import foo.Bar;
x = new Foo();
function g() {}
```

**textparser CST** `tp=28 nodes`

```
Repeat
  ImportStatement
    ImportKeyword
    QualifiedIdentifier
      Variable
      Repeat
        Sequence
          ObjectMember
          Variable
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
    Semicolon
  FunctionDeclaration
    FunctionKeyword
    Variable
    LParen
    RParen
    BlockStatement
      LBrace
      RBrace
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    import_statement
      import_path
        identifier
        identifier
    expression_statement
      assignment_expression
        identifier
        new_expression
          identifier
          arguments
    function_declaration
      identifier
      formal_parameters
      statement_block
```

### script-corpus / case_insensitivity.txt / 16: all four casing forms

```
Return 1;
RETURN 2;
return 3;
```

**textparser CST** `tp=13 nodes`

```
Repeat
  ReturnStatement
    ReturnKeyword
    Number
    Semicolon
  ReturnStatement
    ReturnKeyword
    Number
    Semicolon
  ReturnStatement
    ReturnKeyword
    Number
    Semicolon
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    return_statement
      number
    return_statement
      number
    return_statement
      number
```

### script-corpus / case_insensitivity.txt / 17: camelCase keyword accepts all four forms

```
a = queryExecute("SELECT 1");
b = QueryExecute("SELECT 1");
c = QUERYEXECUTE("SELECT 1");
d = queryexecute("SELECT 1");
```

**textparser CST** `tp=45 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
    Semicolon
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        query_expression
    expression_statement
      assignment_expression
        identifier
        query_expression
    expression_statement
      assignment_expression
        identifier
        query_expression
    expression_statement
      assignment_expression
        identifier
        query_expression
```

### script-corpus / case_insensitivity.txt / 19: parameter type casing

```
function a(string s, numeric n, boolean b, date d) {}
function b(STRING s, NUMERIC n, BOOLEAN b, DATE d) {}
function c(String s, Numeric n, Boolean b, Date d) {}
function d(any a, xml x, binary bin, guid g, void v) {}
```

**textparser CST** `tp=118 nodes`

```
Repeat
  FunctionDeclaration
    FunctionKeyword
    Variable
    LParen
    ParameterList
      Parameter
        Variable
        Variable
      Repeat
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
            Variable
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    FunctionKeyword
    Variable
    LParen
    ParameterList
      Parameter
        Variable
        Variable
      Repeat
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
            Variable
    RParen
    BlockStatement
      LBrace
      RBrace
  FunctionDeclaration
    FunctionKeyword
    Variable
    LParen
    ParameterList
      Parameter
        Variable
        Variable
      Repeat
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
            Variable
        Sequence
          Separator
          Parameter
            Variable
```

**tree-sitter CST** `ts=68 nodes`

```
  program
    function_declaration
      identifier
      formal_parameters
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
      statement_block
    function_declaration
      identifier
      formal_parameters
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
      statement_block
    function_declaration
      identifier
      formal_parameters
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
      statement_block
    function_declaration
      identifier
      formal_parameters
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
      statement_block
```

### script-corpus / case_insensitivity.txt / 20: instanceOf operator casing

```
a = x instanceOf y;
b = x instanceof y;
c = x INSTANCEOF y;
```

**textparser CST** `tp=28 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
    Semicolon
```

**tree-sitter CST** `ts=19 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        binary_expression
          identifier
          identifier
    expression_statement
      assignment_expression
        identifier
        binary_expression
          identifier
          identifier
    expression_statement
      assignment_expression
        identifier
        binary_expression
          identifier
          identifier
```

### script-corpus / cfscript.txt / 0: cfscript comments

```
    // test
    test(); // test

    if ( test EQ test ) {
       test();
    }
    /* test */
```

**textparser CST** `tp=28 nodes`

```
ScriptTagPair
  ScriptStartTag_Start
  TagEnd
  Repeat
    ExpressionStatement
      PostfixExpressionSuffix
        Variable
        LParen
        RParen
      Semicolon
    IfStatement
      IfKeyword
      LParen
      CompareOperator
        Variable
        Variable
      RParen
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
            Semicolon
        RBrace
  ScriptEndTag
```

**tree-sitter CST** `ts=18 nodes`

```
  program
    comment
    expression_statement
      call_expression
        identifier
        arguments
    comment
    if_statement
      parenthesized_expression
        binary_expression
          identifier
          identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
    comment
```

### script-corpus / cfscript.txt / 1: cfscript

```
  cars = fn("abc,test","abc");

  cars = fn(test="#abc#",abc="test",abc);

  cars = fn["Test"](test="abc",abc="test");

  test().test();

  x = 2;
  y = "#x#";
  z = #y#;

  writeDump([x, y, z]);
```

**textparser CST** `tp=118 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
          Repeat
            Sequence
              Separator
              Argument
                DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            AssignOperator
              Variable
              DoubleString
          Repeat
            Sequence
              Separator
              Argument
                AssignOperator
                  Variable
                  DoubleString
            Sequence
              Separator
              Argument
                Variable
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LBracket
          DoubleString
          RBracket
        LParen
        ArgumentList
          Argument
            AssignOperator
              Variable
              DoubleString
          Repeat
            Sequence
              Separator
              Argument
                AssignOperator
                  Variable
                  DoubleString
        RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        ObjectMember
        Variable
      LParen
      RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
```

**tree-sitter CST** `ts=69 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
            string
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            assignment_expression
              identifier
              string
              #
              #
            assignment_expression
              identifier
              string
            identifier
    expression_statement
      assignment_expression
        identifier
        call_expression
          subscript_expression
            identifier
            string
          arguments
            assignment_expression
              identifier
              string
            assignment_expression
              identifier
              string
    expression_statement
      call_expression
        member_expression
          call_expression
            identifier
            arguments
          property_identifier
        arguments
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        identifier
        string
        #
        #
    expression_statement
      assignment_expression
        identifier
        hash_expression
          identifier
    expression_statement
      call_expression
        identifier
        arguments
          array
            identifier
            identifier
            identifier
```

### script-corpus / cfscript.txt / 2: cfscript switch

```
  // test
  switch (test) {
    case "1":
      test();//test
      
      break;
    default: //test
      test();
  }
```

**textparser CST** `tp=32 nodes`

```
SwitchStatement
  SwitchKeyword
  LParen
  Variable
  RParen
  LBrace
  Repeat
    SwitchCase
      CaseKeyword
      DoubleString
      Colon
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
        BreakStatement
          BreakKeyword
          Semicolon
    SwitchCase
      DefaultKeyword
      Colon
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
  RBrace
```

**tree-sitter CST** `ts=20 nodes`

```
  program
    comment
    switch_statement
      parenthesized_expression
        identifier
      switch_body
        switch_case
          string
          expression_statement
            call_expression
              identifier
              arguments
          comment
          break_statement
        switch_default
          comment
          expression_statement
            call_expression
              identifier
              arguments
```

### script-corpus / cfscript.txt / 3: cfscript function 1

```
public string function doThing() 
{

}
```

**textparser CST** `tp=11 nodes`

```
FunctionDeclaration
  Repeat
    PublicKeyword
  Variable
  FunctionKeyword
  Variable
  LParen
  RParen
  BlockStatement
    LBrace
    RBrace
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    function_declaration
      access_type
      identifier
      identifier
      formal_parameters
      statement_block
```

### script-corpus / cfscript.txt / 4: cfscript function 2

```
public string function doThing(string firstName = "blah",required string firstName,string firstName)
{

}
```

**textparser CST** `tp=30 nodes`

```
FunctionDeclaration
  Repeat
    PublicKeyword
  Variable
  FunctionKeyword
  Variable
  LParen
  ParameterList
    Parameter
      Variable
      Variable
      Sequence
        AssignOperator
        DoubleString
    Repeat
      Sequence
        Separator
        Parameter
          RequiredKeyword
          Variable
          Variable
      Sequence
        Separator
        Parameter
          Variable
          Variable
  RParen
  BlockStatement
    LBrace
    RBrace
```

**tree-sitter CST** `ts=18 nodes`

```
  program
    function_declaration
      access_type
      identifier
      identifier
      formal_parameters
        parameter_type
          identifier
        assignment_pattern
          identifier
          string
        parameter_type
          identifier
        identifier
        parameter_type
          identifier
        identifier
      statement_block
```

### script-corpus / cfscript.txt / 7: script tag

```
cfmail(test=test){

};
```

**textparser CST** `tp=15 nodes`

```
Repeat
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          AssignOperator
            Variable
            Variable
      RParen
  BlockStatement
    LBrace
    RBrace
  Semicolon
```

**tree-sitter CST** `ts=8 nodes`

```
  program
    tag_statement
      identifier
      arguments
        assignment_expression
          identifier
          identifier
      statement_block
```

### script-corpus / cfscript.txt / 10: try test

```
try {
			assertTrue(isNull(null));
  
      assertEquals(structKeyExists(outputSetting, "cfmlWriter") && (outputsetting.cfmlwriter EQ "white-space"), true);
        
			var x=null;
			assertTrue(isNull(x));
		}
		finally {
			application action="update" NULLSupport=ns;
		}
    
```

**textparser CST** `tp=102 nodes`

```
ScriptTagPair
  ScriptStartTag_Start
  TagEnd
  Repeat
    TryCatchStatement
      TryKeyword
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    LParen
                    ArgumentList
                      Argument
                        NullKeyword
                    RParen
              RParen
            Semicolon
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  LogicalAndOperator
                    PostfixExpressionSuffix
                      Variable
                      LParen
                      ArgumentList
                        Argument
                          Variable
                        Repeat
                          Sequence
                            Separator
                            Argument
                              DoubleString
                      RParen
                    ParenthesizedExpression
                      LParen
                      CompareOperator
                        PostfixExpressionSuffix
                          Variable
                          ObjectMember
                          Variable
                        DoubleString
                      RParen
                Repeat
                  Sequence
                    Separator
                    Argument
                      Boolean
              RParen
            Semicolon
          VariableDeclarationStatement
            VarKeyword
            VariableDeclaratorList
              VariableDeclarator
                Variable
                Sequence
                  AssignOperator
                  NullKeyword
            Semicolon
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    LParen
                    ArgumentList
                      Argument
                        Variable
```

**tree-sitter CST** `ts=50 nodes`

```
  program
    try_statement
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
              call_expression
                identifier
                arguments
                  null
        expression_statement
          call_expression
            identifier
            arguments
              binary_expression
                call_expression
                  identifier
                  arguments
                    identifier
                    string
                parenthesized_expression
                  binary_expression
                    member_expression
                      identifier
                      property_identifier
                    string
              true
        variable_declaration
          variable_declarator
            identifier
            null
        expression_statement
          call_expression
            identifier
            arguments
              call_expression
                identifier
                arguments
                  identifier
      finally_clause
        statement_block
          tag_statement
            identifier
            assignment_expression
              identifier
              string
            assignment_expression
              identifier
              identifier
```

### script-corpus / cfscript.txt / 13: query test / multiline tags

```
query name="local.qDebugEntriesFiltered" dbtype="query" {
    echo("SELECT * FROM qDebugEntries WHERE label = 'testDebug'")
}

http
  test="test"
  test2="Test";
```

**textparser CST** `tp=34 nodes`

```
Repeat
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              DoubleString
          RParen
    RBrace
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
    Semicolon
```

**tree-sitter CST** `ts=22 nodes`

```
  program
    query_tag
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
              string
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 14: try / catch

```
try{
  // This fails, if prev statement updates the password for server admin.
  admin.updatePassword(oldPassword="#request.ServerAdminPassword#", newPassword="server" );
}catch( any e ){
  errorTemplate.template404 = errorGet.templates.404;
  assertEquals( e.message, 'No access, password is invalid' );
}
```

**textparser CST** `tp=68 nodes`

```
TryCatchStatement
  TryKeyword
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            Variable
            ObjectMember
            Variable
          LParen
          ArgumentList
            Argument
              AssignOperator
                Variable
                DoubleString
            Repeat
              Sequence
                Separator
                Argument
                  AssignOperator
                    Variable
                    DoubleString
          RParen
        Semicolon
    RBrace
  Repeat
    CatchClause
      CatchKeyword
      LParen
      Variable
      Variable
      RParen
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            AssignOperator
              PostfixExpressionSuffix
                Variable
                ObjectMember
                Variable
              PostfixExpressionSuffix
                Variable
                ObjectMember
                Variable
          ExpressionStatement
            Number
            Semicolon
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
                Repeat
                  Sequence
                    Separator
                    Argument
                      SingleString
              RParen
            Semicolon
        RBrace
```

**tree-sitter CST** `ts=42 nodes`

```
  program
    try_statement
      statement_block
        comment
        expression_statement
          call_expression
            member_expression
              identifier
              property_identifier
            arguments
              assignment_expression
                identifier
                string
                #
                .
                #
              assignment_expression
                identifier
                string
      catch_clause
        catch_type
        identifier
        statement_block
          expression_statement
            assignment_expression
              member_expression
                identifier
                property_identifier
              member_expression
                member_expression
                  identifier
                  property_identifier
                property_identifier
          expression_statement
            call_expression
              identifier
              arguments
                member_expression
                  identifier
                  property_identifier
                string
                  string_fragment
```

### script-corpus / cfscript.txt / 16: for in with uppercase IN

```
for (item IN collection) {

}
```

**textparser CST** `tp=10 nodes`

```
ForInStatement
  ForKeyword
  LParen
  Variable
  InKeyword
  Variable
  RParen
  BlockStatement
    LBrace
    RBrace
```

**tree-sitter CST** `ts=5 nodes`

```
  program
    for_in_statement
      identifier
      identifier
      statement_block
```

### script-corpus / cfscript.txt / 17: javascript escape sequences that don't work in CF

```
var mapping = ListChangeDelims( arguments.testMapping, "", "/\" );
```

**textparser CST** `tp=27 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              PostfixExpressionSuffix
                Variable
                ObjectMember
                Variable
            Repeat
              Sequence
                Separator
                Argument
                  DoubleString
              Sequence
                Separator
                Argument
                  DoubleString
          RParen
  Semicolon
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        call_expression
          identifier
          arguments
            member_expression
              identifier
              property_identifier
            string
            string
```

### script-corpus / cfscript.txt / 18: tag statements

```
silent {
		test();
}

imap
    action = "close",
    connection="testImap";
```

**textparser CST** `tp=25 nodes`

```
Repeat
  ExpressionStatement
    Variable
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        Semicolon
    RBrace
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  Statement
    Separator
    Variable
    AssignOperator
    DoubleString
  Semicolon
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    tag_statement
      identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 19: ternary / elvis operator / cfml comments / regex

```
test ?: "Test";
test ? "test" : "Test";
test = {type="perl"};

src = reReplace(src, "<!---.*?--->", "", "all"); // cfml comments
src = reReplace(src, "\/{2}.*|\/\*[\s\S]*?\*\/", "", "all"); // regex
  
```

**textparser CST** `tp=76 nodes`

```
ScriptTagPair
  ScriptStartTag_Start
  TagEnd
  Repeat
    ExpressionStatement
      ElvisOperator
        Variable
        DoubleString
      Semicolon
    ExpressionStatement
      Question
        Variable
        DoubleString
        DoubleString
      Semicolon
    ExpressionStatement
      AssignOperator
        Variable
        StructLiteral
          LBrace
          StructMemberList
            StructMember
              Variable
              AssignOperator
              DoubleString
          RBrace
      Semicolon
    ExpressionStatement
      AssignOperator
        Variable
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
            Repeat
              Sequence
                Separator
                Argument
                  DoubleString
              Sequence
                Separator
                Argument
                  DoubleString
              Sequence
                Separator
                Argument
                  DoubleString
          RParen
      Semicolon
    ExpressionStatement
      AssignOperator
        Variable
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              Variable
            Repeat
              Sequence
                Separator
                Argument
                  DoubleString
              Sequence
                Separator
                Argument
                  DoubleString
              Sequence
                Separator
                Argument
                  DoubleString
          RParen
      Semicolon
  ScriptEndTag
```

**tree-sitter CST** `ts=39 nodes`

```
  program
    expression_statement
      elvis_expression
        identifier
        string
    expression_statement
      ternary_expression
        identifier
        string
        string
    expression_statement
      assignment_expression
        identifier
        object_pattern
          object_assignment_pattern
            shorthand_property_identifier_pattern
            string
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            identifier
            string
            string
            string
    comment
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            identifier
            string
            string
            string
    comment
```

### script-corpus / cfscript.txt / 21: Lucee built-in functions (all parse as call_expression)

```
  get();
  set();
  array(1);
  structNew();
  queryNew("col");
  writeOutput("");
  arrayLen([]);
  isDefined("x");
  de("x");
  createObject("java", "java.lang.String");
```

**textparser CST** `tp=89 nodes`

```
Repeat
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          ArrayLiteral
            LBracket
            RBracket
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
```

**tree-sitter CST** `ts=49 nodes`

```
  program
    expression_statement
      call_expression
        identifier
        arguments
    expression_statement
      call_expression
        identifier
        arguments
    expression_statement
      call_expression
        identifier
        arguments
          number
    expression_statement
      call_expression
        identifier
        arguments
    expression_statement
      call_expression
        identifier
        arguments
          string
    expression_statement
      call_expression
        identifier
        arguments
          string
    expression_statement
      call_expression
        identifier
        arguments
          array
    expression_statement
      call_expression
        identifier
        arguments
          string
    expression_statement
      call_expression
        identifier
        arguments
          string
    expression_statement
      call_expression
        identifier
        arguments
          string
          string
```

### script-corpus / cfscript.txt / 23: queryExecute test

```
var data = queryExecute("
    SELECT courseId
    FROM Courses AS c
        JOIN Employees AS i ON c.instructorId = i.employeeId
    WHERE c.startDate BETWEEN :startDate AND :endDate AND c.statusId < 4
        " & sql,
{
    startDate   : { cfsqltype: "date", value: startDate },
    endDate     : { cfsqltype: "date", value: endDate.add("d", 1) },
    instructorId: { cfsqltype: "integer", value: instructorId },
    courseDate  : { cfsqltype: "date", value: courseDate }
});
```

**textparser CST** `tp=118 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              ConcatOperator
                DoubleString
                Variable
            Repeat
              Sequence
                Separator
                Argument
                  StructLiteral
                    LBrace
                    StructMemberList
                      StructMember
                        Variable
                        Colon
                        StructLiteral
                          LBrace
                          StructMemberList
                            StructMember
                              Variable
                              Colon
                              DoubleString
                            Repeat
                              Sequence
                                Separator
                                StructMember
                                  Variable
                                  Colon
                                  Variable
                          RBrace
                      Repeat
                        Sequence
                          Separator
                          StructMember
                            Variable
                            Colon
                            StructLiteral
                              LBrace
                              StructMemberList
                                StructMember
                                  Variable
                                  Colon
                                  DoubleString
                                Repeat
                                  Sequence
                                    Separator
                                    StructMember
                                      Variable
                                      Colon
                                      PostfixExpressionSuffix
                                        PostfixExpressionSuffix
                                          Variable
                                          ObjectMember
                                          Variable
                                        LParen
                                        ArgumentList
                                          Argument
                                            DoubleString
                                          Repeat
                                            Sequence
                                              Separator
                                              Argument
                                                Number
                                        RParen
                              RBrace
                        Sequence
                          Separator
                          StructMember
                            Variable
                            Colon
```

**tree-sitter CST** `ts=49 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        query_expression
          identifier
          object
            pair
              property_identifier
              object
                pair
                  property_identifier
                  string
                pair
                  property_identifier
                  identifier
            pair
              property_identifier
              object
                pair
                  property_identifier
                  string
                pair
                  property_identifier
                  call_expression
                    member_expression
                      identifier
                      property_identifier
                    arguments
                      string
                      number
            pair
              property_identifier
              object
                pair
                  property_identifier
                  string
                pair
                  property_identifier
                  identifier
            pair
              property_identifier
              object
                pair
                  property_identifier
                  string
                pair
                  property_identifier
                  identifier
```

### script-corpus / cfscript.txt / 34: savecontent with include

```
savecontent variable="captured" {
    include "styles.css";
}
```

**textparser CST** `tp=15 nodes`

```
Repeat
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  BlockStatement
    LBrace
    Repeat
      IncludeStatement
        IncludeKeyword
        DoubleString
        Semicolon
    RBrace
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      statement_block
        include_statement
          string
```

### script-corpus / cfscript.txt / 47: query variable method chaining

```
query
    .newQuery()
    .from("table")
    .get();
```

**textparser CST** `tp=24 nodes`

```
ExpressionStatement
  PostfixExpressionSuffix
    PostfixExpressionSuffix
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            LParen
            RParen
          ObjectMember
          Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
      ObjectMember
      Variable
    LParen
    RParen
  Semicolon
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    expression_statement
      call_expression
        member_expression
          call_expression
            member_expression
              call_expression
                member_expression
                  identifier
                  property_identifier
                arguments
              property_identifier
            arguments
              string
          property_identifier
        arguments
```

### script-corpus / cfscript.txt / 54: import with wildcard

```
import component.*;
```

**textparser CST** `tp=7 nodes`

```
Repeat
  Statement
    ImportKeyword
    ComponentKeyword
    ObjectMember
    MulOperator
  Semicolon
```

**tree-sitter CST** `ts=4 nodes`

```
  program
    import_statement
      import_path
        identifier
```

### script-corpus / cfscript.txt / 55: multiline query tag with cachedWithin

```
query
	        name="local.qoq"
	        dbtype="query"
	        cachedWithin=createTimeSpan(0,0,0,time) {
	        echo('select * from qry where a>'&time);
	    }
```

**textparser CST** `tp=49 nodes`

```
Repeat
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            Number
          Repeat
            Sequence
              Separator
              Argument
                Number
            Sequence
              Separator
              Argument
                Number
            Sequence
              Separator
              Argument
                Variable
        RParen
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              ConcatOperator
                SingleString
                Variable
          RParen
        Semicolon
    RBrace
```

**tree-sitter CST** `ts=26 nodes`

```
  program
    query_tag
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            number
            number
            number
            identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
              binary_expression
                string
                  string_fragment
                identifier
```

### script-corpus / cfscript.txt / 57: cfml template block after a statement with no semicolon

```
thread name="t" {
	thread.test = "thread";
}

```
	<cfset res = "works">
```
thread action="join" name="t";
```

**textparser CST** `tp=16 nodes`

```
Template
  Statement
    ScriptStartTag_Start
    TagEnd
    ThreadKeyword
    Variable
    AssignOperator
    DoubleString
    LBrace
    ThreadKeyword
    ObjectMember
    Variable
    AssignOperator
    DoubleString
  Semicolon
  RBrace
```

**tree-sitter CST** `ts=23 nodes`

```
  program
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      statement_block
        expression_statement
          assignment_expression
            member_expression
              identifier
              property_identifier
            string
    cfml_template
      cfml_template_content
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 61: new as property name

```
obj.new;
obj.new();
obj.new = "test";
var x = obj.new.toString();
```

**textparser CST** `tp=39 nodes`

```
Repeat
  ExpressionStatement
    Variable
  Statement
    ObjectMember
    NewKeyword
  Semicolon
  ExpressionStatement
    Variable
  Statement
    ObjectMember
    NewKeyword
    LParen
    RParen
  Semicolon
  ExpressionStatement
    Variable
  Statement
    ObjectMember
    NewKeyword
    AssignOperator
    DoubleString
  Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          Variable
  Statement
    ObjectMember
    NewKeyword
    ObjectMember
    Variable
    LParen
    RParen
  Semicolon
```

**tree-sitter CST** `ts=27 nodes`

```
  program
    expression_statement
      member_expression
        identifier
        property_identifier
    expression_statement
      call_expression
        member_expression
          identifier
          property_identifier
        arguments
    expression_statement
      assignment_expression
        member_expression
          identifier
          property_identifier
        string
    variable_declaration
      variable_declarator
        identifier
        call_expression
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
```

### script-corpus / cfscript.txt / 62: pair syntax in named function arguments

```
foo(arg1: "value1", arg2: 42);
```

**textparser CST** `tp=14 nodes`

```
Repeat
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    Colon
    DoubleString
    Separator
    Variable
    Colon
    Number
    RParen
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    expression_statement
      call_expression
        identifier
        arguments
          pair
            property_identifier
            string
          pair
            property_identifier
            number
```

### script-corpus / cfscript.txt / 63: pair syntax in object literal

```
var s = {name: "Alice", age: 30};
```

**textparser CST** `tp=23 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        StructLiteral
          LBrace
          StructMemberList
            StructMember
              Variable
              Colon
              DoubleString
            Repeat
              Sequence
                Separator
                StructMember
                  Variable
                  Colon
                  Number
          RBrace
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        object
          pair
            property_identifier
            string
          pair
            property_identifier
            number
```

### script-corpus / cfscript.txt / 64: pair syntax in ordered struct array

```
var s = [name: "Alice", age: 30];
```

**textparser CST** `tp=23 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        StructLiteral
          LBracket
          StructMemberList
            StructMember
              Variable
              Colon
              DoubleString
            Repeat
              Sequence
                Separator
                StructMember
                  Variable
                  Colon
                  Number
          RBracket
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        array
          pair
            property_identifier
            string
          pair
            property_identifier
            number
```

### script-corpus / cfscript.txt / 65: equals syntax in named function arguments

```
foo(arg1 = "value1", arg2 = 42);
```

**textparser CST** `tp=18 nodes`

```
ExpressionStatement
  PostfixExpressionSuffix
    Variable
    LParen
    ArgumentList
      Argument
        AssignOperator
          Variable
          DoubleString
      Repeat
        Sequence
          Separator
          Argument
            AssignOperator
              Variable
              Number
    RParen
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    expression_statement
      call_expression
        identifier
        arguments
          assignment_expression
            identifier
            string
          assignment_expression
            identifier
            number
```

### script-corpus / cfscript.txt / 66: equals syntax in object literal

```
var s = {name = "Alice", age = 30};
```

**textparser CST** `tp=23 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        StructLiteral
          LBrace
          StructMemberList
            StructMember
              Variable
              AssignOperator
              DoubleString
            Repeat
              Sequence
                Separator
                StructMember
                  Variable
                  AssignOperator
                  Number
          RBrace
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        object_pattern
          object_assignment_pattern
            shorthand_property_identifier_pattern
            string
          object_assignment_pattern
            shorthand_property_identifier_pattern
            number
```

### script-corpus / cfscript.txt / 67: equals syntax in array

```
var s = [name = "Alice", age = 30];
```

**textparser CST** `tp=23 nodes`

```
VariableDeclarationStatement
  VarKeyword
  VariableDeclaratorList
    VariableDeclarator
      Variable
      Sequence
        AssignOperator
        ArrayLiteral
          LBracket
          ArgumentList
            Argument
              AssignOperator
                Variable
                DoubleString
            Repeat
              Sequence
                Separator
                Argument
                  AssignOperator
                    Variable
                    Number
          RBracket
  Semicolon
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        array
          assignment_expression
            identifier
            string
          assignment_expression
            identifier
            number
```

### script-corpus / cfscript.txt / 71: param with the = spelling of the default

```
param numeric shortBad = "abc";
param string url.id = "0";
param numeric x default="0";
param numeric y = 1 max=5;
param name="z" type="numeric" default="0";
```

**textparser CST** `tp=73 nodes`

```
Repeat
  ParamStatement
    ParamKeyword
    Repeat
      TagAttribute
        Variable
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    Semicolon
  ParamStatement
    ParamKeyword
    Repeat
      TagAttribute
        Variable
      TagAttribute
        Variable
  Statement
    ObjectMember
    Variable
    AssignOperator
    DoubleString
  Semicolon
  ParamStatement
    ParamKeyword
    Repeat
      TagAttribute
        Variable
      TagAttribute
        Variable
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          DoubleString
    Semicolon
  ParamStatement
    ParamKeyword
    Repeat
      TagAttribute
        Variable
      TagAttribute
        Variable
        Sequence
          AssignOperator
          Number
      TagAttribute
        Variable
        Sequence
          AssignOperator
          Number
    Semicolon
  ParamStatement
    ParamKeyword
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
      TagAttribute
        DefaultKeyword
        Sequence
          AssignOperator
          DoubleString
    Semicolon
```

**tree-sitter CST** `ts=42 nodes`

```
  program
    tag_statement
      identifier
      parameter_type
      assignment_expression
        identifier
        string
    tag_statement
      identifier
      parameter_type
      assignment_expression
        member_expression
          identifier
          property_identifier
        string
    tag_statement
      identifier
      parameter_type
      identifier
      assignment_expression
        identifier
        string
    tag_statement
      identifier
      parameter_type
      assignment_expression
        identifier
        number
      assignment_expression
        identifier
        number
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 75: ordered struct literal, and $ as an ordinary variable (#80)

```

animals = ${ Aardwolf: "Proteles cristata", aardvark: "Orycteropus afer" };
empty = ${};
first = $[ 1 ];
$ = 1;
sel = $( "x" );
```

**textparser CST** `tp=52 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        Variable
      Statement
        Colon
        DoubleString
        Separator
        Variable
        Colon
        DoubleString
    RBrace
  Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  BlockStatement
    LBrace
    RBrace
  Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LBracket
        Number
        RBracket
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Number
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
        RParen
    Semicolon
```

**tree-sitter CST** `ts=32 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        ordered_struct
          pair
            property_identifier
            string
          pair
            property_identifier
            string
    expression_statement
      assignment_expression
        identifier
        ordered_struct
    expression_statement
      assignment_expression
        identifier
        subscript_expression
          identifier
          number
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
```

### script-corpus / cfscript.txt / 77: a built-in type name parenthesised as an expression

```

x = ( date );
y = ( string ) & ( numeric );
z = ( date[ 2 ] > 0 && date[ 2 ] <= ArrayLen( a ) );
v = ( void );

g = ( string s ) => s;
h = ( s ) => s;
```

**textparser CST** `tp=85 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      ParenthesizedExpression
        LParen
        Variable
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ConcatOperator
        ParenthesizedExpression
          LParen
          Variable
          RParen
        ParenthesizedExpression
          LParen
          Variable
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ParenthesizedExpression
        LParen
        LogicalAndOperator
          CompareOperator
            PostfixExpressionSuffix
              Variable
              LBracket
              Number
              RBracket
            Number
          CompareOperator
            PostfixExpressionSuffix
              Variable
              LBracket
              Number
              RBracket
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  Variable
              RParen
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ParenthesizedExpression
        LParen
        Variable
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ArrowFunction
        Sequence
          LParen
          ParameterList
            Parameter
              Variable
              Variable
          RParen
        LambdaOperator
        Variable
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ArrowFunction
        Sequence
          LParen
          ParameterList
            Parameter
```

**tree-sitter CST** `ts=54 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        parenthesized_expression
          identifier
    expression_statement
      assignment_expression
        identifier
        binary_expression
          parenthesized_expression
            identifier
          parenthesized_expression
            identifier
    expression_statement
      assignment_expression
        identifier
        parenthesized_expression
          binary_expression
            binary_expression
              subscript_expression
                identifier
                number
              number
            binary_expression
              subscript_expression
                identifier
                number
              call_expression
                identifier
                arguments
                  identifier
    expression_statement
      assignment_expression
        identifier
        parenthesized_expression
          identifier
    expression_statement
      assignment_expression
        identifier
        arrow_function
          formal_parameters
            parameter_type
              identifier
            identifier
          identifier
    expression_statement
      assignment_expression
        identifier
        arrow_function
          formal_parameters
            parameter_type
              identifier
          identifier
```

### script-corpus / cfscript.txt / 78: var declaration with a compound assignment

```

var jql = "project = x";
var jql &= " ORDER BY key DESC";
var total += getReviews()[ 1 ].getRating();
var local.note &= "b";
final y &= 1;

var a = 1, b = 2;
var mappings[ key ] = value;
var new = 1;
bare &= "no var";
```

**textparser CST** `tp=102 nodes`

```
Repeat
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          DoubleString
    Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          DoubleString
    Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              PostfixExpressionSuffix
                PostfixExpressionSuffix
                  Variable
                  LParen
                  RParen
                LBracket
                Number
                RBracket
              ObjectMember
              Variable
            LParen
            RParen
    Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
  Statement
    ObjectMember
    Variable
    AssignOperator
    DoubleString
  Semicolon
  Statement
    FinalKeyword
    Variable
    AssignOperator
    Number
  Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
        Sequence
          AssignOperator
          Number
      Repeat
        Sequence
          Separator
          VariableDeclarator
            Variable
            Sequence
              AssignOperator
              Number
    Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
```

**tree-sitter CST** `ts=52 nodes`

```
  program
    variable_declaration
      variable_declarator
        identifier
        string
    variable_declaration
      variable_declarator
        identifier
        string
    variable_declaration
      variable_declarator
        identifier
        call_expression
          member_expression
            subscript_expression
              call_expression
                identifier
                arguments
              number
            property_identifier
          arguments
    variable_declaration
      variable_declarator
        member_expression
          identifier
          property_identifier
        string
    variable_declaration
      variable_declarator
        identifier
        number
    variable_declaration
      variable_declarator
        identifier
        number
      variable_declarator
        identifier
        number
    variable_declaration
      variable_declarator
        subscript_expression
          identifier
          identifier
        identifier
    variable_declaration
      variable_declarator
        identifier
        number
    expression_statement
      augmented_assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 92: inline java class block

```

classInstance = java{
	public class class1{
		public String execute() { return "java block worked in Lucee"; }
	}
}
writeoutput(classInstance.execute())
a = java { public class C { String s = "}"; char c = '}'; } };
b = java { /* } */ public class C { } };
c = JAVA { public class D { } };
d = new java.util.Properties();
e = new java:foo.Bar();
java = 1;
f = java.lang.System;
g = createObject("java", "java.util.Date");
h = javaCast("int", 1);
loop array=java { x = 1; }
i = java { class D { } };
j = java { @Deprecated public class E { } };
```

**textparser CST** `tp=200 nodes`

```
Template
  Statement
    ScriptStartTag_Start
    TagEnd
    Variable
    AssignOperator
    Variable
    LBrace
    PublicKeyword
    Variable
    Variable
    LBrace
    PublicKeyword
    Variable
    Variable
    LParen
    RParen
    LBrace
    ReturnKeyword
    DoubleString
  Semicolon
  RBrace
  RBrace
  RBrace
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            LParen
            RParen
      RParen
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  LBrace
  Statement
    PublicKeyword
    Variable
    Variable
    LBrace
    Variable
    Variable
    AssignOperator
    DoubleString
  Semicolon
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      SingleString
    Semicolon
  RBrace
  RBrace
  Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  LBrace
  Statement
    PublicKeyword
    Variable
    Variable
    LBrace
  RBrace
  RBrace
  Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
```

**tree-sitter CST** `ts=97 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        java_class_block
          java_class_content
    expression_statement
      call_expression
        identifier
        arguments
          call_expression
            member_expression
              identifier
              property_identifier
            arguments
    expression_statement
      assignment_expression
        identifier
        java_class_block
          java_class_content
    expression_statement
      assignment_expression
        identifier
        java_class_block
          java_class_content
    expression_statement
      assignment_expression
        identifier
        java_class_block
          java_class_content
    expression_statement
      assignment_expression
        identifier
        new_expression
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
    expression_statement
      assignment_expression
        identifier
        new_expression
          type_prefix
          member_expression
            identifier
            property_identifier
          arguments
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        identifier
        member_expression
          member_expression
            identifier
            property_identifier
          property_identifier
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
            string
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
            number
    tag_statement
      identifier
      assignment_expression
```

### script-corpus / cfscript.txt / 93: subscript assignment is not a tag statement

```

r[ a ][ b ] = new R( v=1 );
r[ a ][ b ] = 1;
interceptors[ x ].name = listLast( interceptors[ x ].class, "." );
modules[ m ].i18n = { defaultLocale = "" };
location url="/home" addtoken="false";
http url="https://x" method="get";
param numeric x default="0";
```

**textparser CST** `tp=121 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LBracket
          Variable
          RBracket
        LBracket
        Variable
        RBracket
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              AssignOperator
                Variable
                Number
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LBracket
          Variable
          RBracket
        LBracket
        Variable
        RBracket
      Number
    Semicolon
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LBracket
          Variable
          RBracket
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            PostfixExpressionSuffix
              PostfixExpressionSuffix
                Variable
                LBracket
                Variable
                RBracket
              ObjectMember
              Variable
          Repeat
            Sequence
              Separator
              Argument
                DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        PostfixExpressionSuffix
          Variable
          LBracket
          Variable
          RBracket
        ObjectMember
        Variable
      StructLiteral
        LBrace
        StructMemberList
```

**tree-sitter CST** `ts=72 nodes`

```
  program
    expression_statement
      assignment_expression
        subscript_expression
          subscript_expression
            identifier
            identifier
          identifier
        new_expression
          identifier
          arguments
            assignment_expression
              identifier
              number
    expression_statement
      assignment_expression
        subscript_expression
          subscript_expression
            identifier
            identifier
          identifier
        number
    expression_statement
      assignment_expression
        member_expression
          subscript_expression
            identifier
            identifier
          property_identifier
        call_expression
          identifier
          arguments
            member_expression
              subscript_expression
                identifier
                identifier
              property_identifier
            string
    expression_statement
      assignment_expression
        member_expression
          subscript_expression
            identifier
            identifier
          property_identifier
        object_pattern
          object_assignment_pattern
            shorthand_property_identifier_pattern
            string
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
    tag_statement
      identifier
      assignment_expression
        identifier
        string
      assignment_expression
        identifier
        string
    tag_statement
      identifier
      parameter_type
      identifier
      assignment_expression
        identifier
        string
```

### script-corpus / cfscript.txt / 95: switch with case and default clauses

```

switch (x) {
	case 1:
		y = 1;
		break;
	case 2:
	case 3:
		y = 2;
		break;
	default:
		y = 3;
}
```

**textparser CST** `tp=47 nodes`

```
SwitchStatement
  SwitchKeyword
  LParen
  Variable
  RParen
  LBrace
  Repeat
    SwitchCase
      CaseKeyword
      Number
      Colon
      Repeat
        ExpressionStatement
          AssignOperator
            Variable
            Number
          Semicolon
        BreakStatement
          BreakKeyword
          Semicolon
    SwitchCase
      CaseKeyword
      Number
      Colon
    SwitchCase
      CaseKeyword
      Number
      Colon
      Repeat
        ExpressionStatement
          AssignOperator
            Variable
            Number
          Semicolon
        BreakStatement
          BreakKeyword
          Semicolon
    SwitchCase
      DefaultKeyword
      Colon
      Repeat
        ExpressionStatement
          AssignOperator
            Variable
            Number
          Semicolon
  RBrace
```

**tree-sitter CST** `ts=26 nodes`

```
  program
    switch_statement
      parenthesized_expression
        identifier
      switch_body
        switch_case
          number
          expression_statement
            assignment_expression
              identifier
              number
          break_statement
        switch_case
          number
        switch_case
          number
          expression_statement
            assignment_expression
              identifier
              number
          break_statement
        switch_default
          expression_statement
            assignment_expression
              identifier
              number
```

### script-corpus / cfscript.txt / 100: numeric struct key in write position (#86)

```

myNumb.4 = "4";
myNumb.4b = "4";
myNumb.4.5 = "x";
a.b.4 = 1;
var myNumb.4 = 1;
x = myNumb.4;
x = .5;
x = 1 + .5;
f(.5);
x = [.5, .25];
x = -.0123456789;
x = .5e3;
x = 1.5;
myNumb[4] = "4";
```

**textparser CST** `tp=114 nodes`

```
Repeat
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Number
      DoubleString
    Semicolon
  ExpressionStatement
    Variable
  ExpressionStatement
    Number
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
    Semicolon
  ExpressionStatement
    Variable
  ExpressionStatement
    Number
  ExpressionStatement
    AssignOperator
      Number
      DoubleString
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  ExpressionStatement
    AssignOperator
      Number
      Number
    Semicolon
  VariableDeclarationStatement
    VarKeyword
    VariableDeclaratorList
      VariableDeclarator
        Variable
  ExpressionStatement
    AssignOperator
      Number
      Number
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Variable
  ExpressionStatement
    Number
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      Number
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      AddOperator
        Number
        Number
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
      RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      ArrayLiteral
        LBracket
        ArgumentList
```

**tree-sitter CST** `ts=82 nodes`

```
  program
    expression_statement
      assignment_expression
        member_expression
          identifier
          property_identifier
        string
    expression_statement
      assignment_expression
        member_expression
          identifier
          property_identifier
        string
    expression_statement
      assignment_expression
        member_expression
          member_expression
            identifier
            property_identifier
          property_identifier
        string
    expression_statement
      assignment_expression
        member_expression
          member_expression
            identifier
            property_identifier
          property_identifier
        number
    variable_declaration
      variable_declarator
        member_expression
          identifier
          property_identifier
        number
    expression_statement
      assignment_expression
        identifier
        member_expression
          identifier
          property_identifier
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        identifier
        binary_expression
          number
          number
    expression_statement
      call_expression
        identifier
        arguments
          number
    expression_statement
      assignment_expression
        identifier
        array
          number
          number
    expression_statement
      assignment_expression
        identifier
        unary_expression
          unary_operator
          number
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        identifier
        number
    expression_statement
      assignment_expression
        subscript_expression
          identifier
```

### script-corpus / common.txt / 4: common: queryExecute with params and options

```

q = queryExecute(
	"SELECT id FROM t WHERE a = :a",
	{ a: 1 },
	{ datasource: "ds" }
);
```

**textparser CST** `tp=34 nodes`

```
ExpressionStatement
  AssignOperator
    Variable
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
        Repeat
          Sequence
            Separator
            Argument
              StructLiteral
                LBrace
                StructMemberList
                  StructMember
                    Variable
                    Colon
                    Number
                RBrace
          Sequence
            Separator
            Argument
              StructLiteral
                LBrace
                StructMemberList
                  StructMember
                    Variable
                    Colon
                    DoubleString
                RBrace
      RParen
  Semicolon
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        query_expression
          object
            pair
              property_identifier
              number
          object
            pair
              property_identifier
              string
```

### script-corpus / common.txt / 5: common: try catch finally

```

try {
	risky();
} catch ( any e ) {
	log( e );
} finally {
	cleanup();
}
```

**textparser CST** `tp=44 nodes`

```
TryCatchStatement
  TryKeyword
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        Semicolon
    RBrace
  Repeat
    CatchClause
      CatchKeyword
      LParen
      Variable
      Variable
      RParen
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  Variable
              RParen
            Semicolon
        RBrace
  Sequence
    FinallyKeyword
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
      RBrace
```

**tree-sitter CST** `ts=22 nodes`

```
  program
    try_statement
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
      catch_clause
        catch_type
        identifier
        statement_block
          expression_statement
            call_expression
              identifier
              arguments
                identifier
      finally_clause
        statement_block
          expression_statement
            call_expression
              identifier
              arguments
```

### script-corpus / common.txt / 9: common: nested struct and array literals

```

cfg = {
	list: [ 1, 2, { a: "b" } ],
	"quoted key": true,
	nested: { deep: [ { x: 1 } ] }
};
```

**textparser CST** `tp=65 nodes`

```
ExpressionStatement
  AssignOperator
    Variable
    StructLiteral
      LBrace
      StructMemberList
        StructMember
          Variable
          Colon
          ArrayLiteral
            LBracket
            ArgumentList
              Argument
                Number
              Repeat
                Sequence
                  Separator
                  Argument
                    Number
                Sequence
                  Separator
                  Argument
                    StructLiteral
                      LBrace
                      StructMemberList
                        StructMember
                          Variable
                          Colon
                          DoubleString
                      RBrace
            RBracket
        Repeat
          Sequence
            Separator
            StructMember
              DoubleString
              Colon
              Boolean
          Sequence
            Separator
            StructMember
              Variable
              Colon
              StructLiteral
                LBrace
                StructMemberList
                  StructMember
                    Variable
                    Colon
                    ArrayLiteral
                      LBracket
                      ArgumentList
                        Argument
                          StructLiteral
                            LBrace
                            StructMemberList
                              StructMember
                                Variable
                                Colon
                                Number
                            RBrace
                      RBracket
                RBrace
      RBrace
  Semicolon
```

**tree-sitter CST** `ts=27 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        object
          pair
            property_identifier
            array
              number
              number
              object
                pair
                  property_identifier
                  string
          pair
            string
            true
          pair
            property_identifier
            object
              pair
                property_identifier
                array
                  object
                    pair
                      property_identifier
                      number
```

### script-corpus / common.txt / 10: common: script-syntax lock, transaction and thread

```

function f() {
	lock name="l" timeout=10 type="exclusive" {
		x = 1;
	}
	transaction {
		save();
	}
	thread name="t" {
		work();
	}
}
```

**textparser CST** `tp=66 nodes`

```
FunctionDeclaration
  FunctionKeyword
  Variable
  LParen
  RParen
  BlockStatement
    LBrace
    Repeat
      LockStatement
        LockKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              Number
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              AssignOperator
                Variable
                Number
              Semicolon
          RBrace
      TransactionStatement
        TransactionKeyword
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              PostfixExpressionSuffix
                Variable
                LParen
                RParen
              Semicolon
          RBrace
      ThreadStatement
        ThreadKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              PostfixExpressionSuffix
                Variable
                LParen
                RParen
              Semicolon
          RBrace
    RBrace
```

**tree-sitter CST** `ts=38 nodes`

```
  program
    function_declaration
      identifier
      formal_parameters
      statement_block
        tag_statement
          identifier
          assignment_expression
            identifier
            string
          assignment_expression
            identifier
            number
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              assignment_expression
                identifier
                number
        tag_statement
          identifier
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
        tag_statement
          identifier
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
```

### script-corpus / common.txt / 11: common: script-syntax savecontent

```

function f() {
	savecontent variable="s" {
		writeOutput( "hi" );
	}
	return s;
}
```

**textparser CST** `tp=32 nodes`

```
FunctionDeclaration
  FunctionKeyword
  Variable
  LParen
  RParen
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        Variable
      ExpressionStatement
        AssignOperator
          Variable
          DoubleString
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  DoubleString
              RParen
            Semicolon
        RBrace
      ReturnStatement
        ReturnKeyword
        Variable
        Semicolon
    RBrace
```

**tree-sitter CST** `ts=18 nodes`

```
  program
    function_declaration
      identifier
      formal_parameters
      statement_block
        tag_statement
          identifier
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  string
        return_statement
          identifier
```

### script-corpus / common.txt / 12: common: script-syntax loop forms

```

function f() {
	loop from=1 to=10 index="i" { echo( i ); }
	loop array=data item="x" { echo( x ); }
	loop query=q { echo( q.id ); }
	loop list="a,b" index="v" { echo( v ); }
}
```

**textparser CST** `tp=104 nodes`

```
FunctionDeclaration
  FunctionKeyword
  Variable
  LParen
  RParen
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        Variable
      ExpressionStatement
        AssignOperator
          Variable
          Number
      ExpressionStatement
        AssignOperator
          Variable
          Number
      ExpressionStatement
        AssignOperator
          Variable
          DoubleString
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  Variable
              RParen
            Semicolon
        RBrace
      ExpressionStatement
        Variable
      ExpressionStatement
        AssignOperator
          Variable
          Variable
      ExpressionStatement
        AssignOperator
          Variable
          DoubleString
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  Variable
              RParen
            Semicolon
        RBrace
      ExpressionStatement
        Variable
      ExpressionStatement
        AssignOperator
          Variable
          Variable
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
              RParen
            Semicolon
        RBrace
```

**tree-sitter CST** `ts=63 nodes`

```
  program
    function_declaration
      identifier
      formal_parameters
      statement_block
        tag_statement
          identifier
          assignment_expression
            identifier
            number
          assignment_expression
            identifier
            number
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  identifier
        tag_statement
          identifier
          assignment_expression
            identifier
            identifier
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  identifier
        tag_statement
          identifier
          assignment_expression
            identifier
            identifier
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  member_expression
                    identifier
                    property_identifier
        tag_statement
          identifier
          assignment_expression
            identifier
            string
          assignment_expression
            identifier
            string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  identifier
```

### script-corpus / common.txt / 13: common: for, for-in, while and do-while

```

for ( var i = 1; i <= 10; i++ ) { x(); }
for ( var k in data ) { y( k ); }
while ( a ) { b(); }
do { c(); } while ( d );
```

**textparser CST** `tp=83 nodes`

```
Repeat
  StandardForStatement
    ForKeyword
    LParen
    ForInitVariableDeclaration
      VarKeyword
      VariableDeclaratorList
        VariableDeclarator
          Variable
          Sequence
            AssignOperator
            Number
    Semicolon
    CompareOperator
      Variable
      Number
    Semicolon
    IncDecOperator
      Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
      RBrace
  ForInStatement
    ForKeyword
    LParen
    VarKeyword
    Variable
    InKeyword
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                Variable
            RParen
          Semicolon
      RBrace
  WhileStatement
    WhileKeyword
    LParen
    Variable
    RParen
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
      RBrace
  DoWhileStatement
    DoKeyword
    BlockStatement
      LBrace
      Repeat
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            RParen
          Semicolon
      RBrace
    WhileKeyword
    LParen
```

**tree-sitter CST** `ts=41 nodes`

```
  program
    for_statement
      variable_declaration
        variable_declarator
          identifier
          number
      binary_expression
        identifier
        number
      update_expression
        identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
    for_in_statement
      identifier
      identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
              identifier
    while_statement
      parenthesized_expression
        identifier
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
    do_statement
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
      parenthesized_expression
        identifier
```

### script-corpus / common.txt / 14: common: named arguments and argumentCollection

```

f( a = 1, b = "x" );
g( argumentCollection = args );
h( "positional", named = true );
```

**textparser CST** `tp=46 nodes`

```
Repeat
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          AssignOperator
            Variable
            Number
        Repeat
          Sequence
            Separator
            Argument
              AssignOperator
                Variable
                DoubleString
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          AssignOperator
            Variable
            Variable
      RParen
    Semicolon
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          DoubleString
        Repeat
          Sequence
            Separator
            Argument
              AssignOperator
                Variable
                Boolean
      RParen
    Semicolon
```

**tree-sitter CST** `ts=26 nodes`

```
  program
    expression_statement
      call_expression
        identifier
        arguments
          assignment_expression
            identifier
            number
          assignment_expression
            identifier
            string
    expression_statement
      call_expression
        identifier
        arguments
          assignment_expression
            identifier
            identifier
    expression_statement
      call_expression
        identifier
        arguments
          string
          assignment_expression
            identifier
            true
```

### script-corpus / common.txt / 15: common: new and createObject

```

a = new Foo();
b = new path.to.Bar( 1, 2 );
c = createObject( "component", "path.Baz" );
d = createObject( "java", "java.lang.String" );
```

**textparser CST** `tp=67 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            ObjectMember
            Variable
          LParen
          ArgumentList
            Argument
              Number
            Repeat
              Sequence
                Separator
                Argument
                  Number
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
          Repeat
            Sequence
              Separator
              Argument
                DoubleString
        RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            DoubleString
          Repeat
            Sequence
              Separator
              Argument
                DoubleString
        RParen
    Semicolon
```

**tree-sitter CST** `ts=35 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        new_expression
          identifier
          arguments
    expression_statement
      assignment_expression
        identifier
        new_expression
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
            number
            number
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
            string
    expression_statement
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            string
            string
```

### script-corpus / common.txt / 16: common: throw with named arguments

```

try {
	x();
} catch ( any e ) {
	throw( type = "Custom", message = "bad", detail = e.message );
}
```

**textparser CST** `tp=42 nodes`

```
TryCatchStatement
  TryKeyword
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        Semicolon
    RBrace
  Repeat
    CatchClause
      CatchKeyword
      LParen
      Variable
      Variable
      RParen
      BlockStatement
        LBrace
        Repeat
          ThrowStatement
            ThrowKeyword
          Statement
            LParen
            Variable
            AssignOperator
            DoubleString
            Separator
            Variable
            AssignOperator
            DoubleString
            Separator
            Variable
            AssignOperator
            Variable
            ObjectMember
            Variable
            RParen
          Semicolon
        RBrace
```

**tree-sitter CST** `ts=24 nodes`

```
  program
    try_statement
      statement_block
        expression_statement
          call_expression
            identifier
            arguments
      catch_clause
        catch_type
        identifier
        statement_block
          throw_statement
            arguments
              assignment_expression
                identifier
                string
              assignment_expression
                identifier
                string
              assignment_expression
                identifier
                member_expression
                  identifier
                  property_identifier
```

### script-corpus / common.txt / 17: common: import and include statements

```

import foo.Bar;
include "header.cfm";
```

**textparser CST** `tp=14 nodes`

```
Repeat
  ImportStatement
    ImportKeyword
    QualifiedIdentifier
      Variable
      Repeat
        Sequence
          ObjectMember
          Variable
    Semicolon
  IncludeStatement
    IncludeKeyword
    DoubleString
    Semicolon
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    import_statement
      import_path
        identifier
        identifier
    include_statement
      string
```

### script-corpus / common.txt / 18: common: javadoc annotations before a function

```

/**
 * @hint Does a thing
 * @arg.hint The argument
 */
function doThing( required string arg ) {
	return arg;
}
```

**textparser CST** `tp=18 nodes`

```
FunctionDeclaration
  FunctionKeyword
  Variable
  LParen
  ParameterList
    Parameter
      RequiredKeyword
      Variable
      Variable
  RParen
  BlockStatement
    LBrace
    Repeat
      ReturnStatement
        ReturnKeyword
        Variable
        Semicolon
    RBrace
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    comment
    function_declaration
      identifier
      formal_parameters
        parameter_type
          identifier
        identifier
      statement_block
        return_statement
          identifier
```

### script-corpus / common.txt / 19: common: hash interpolation inside strings

```

msg  = "Hello #user.name#, you have #count# items";
path = "/a/#b#/c";
```

**textparser CST** `tp=11 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
    Semicolon
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        string
        #
        .
        #
        #
        #
    expression_statement
      assignment_expression
        identifier
        string
        #
        #
```

### script-corpus / common.txt / 25: common: script-syntax cfdocument and cfhttp

```

function f() {
	cfdocument( format="pdf" ) {
		writeOutput( "hi" );
	}
	cfhttp( url="https://x", method="get", result="r" ) {
		cfhttpparam( type="header", name="A", value="1" );
	}
}
```

**textparser CST** `tp=83 nodes`

```
FunctionDeclaration
  FunctionKeyword
  Variable
  LParen
  RParen
  BlockStatement
    LBrace
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              AssignOperator
                Variable
                DoubleString
          RParen
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  DoubleString
              RParen
            Semicolon
        RBrace
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              AssignOperator
                Variable
                DoubleString
            Repeat
              Sequence
                Separator
                Argument
                  AssignOperator
                    Variable
                    DoubleString
              Sequence
                Separator
                Argument
                  AssignOperator
                    Variable
                    DoubleString
          RParen
      BlockStatement
        LBrace
        Repeat
          ExpressionStatement
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  AssignOperator
                    Variable
                    DoubleString
                Repeat
                  Sequence
                    Separator
                    Argument
                      AssignOperator
                        Variable
                        DoubleString
                  Sequence
                    Separator
                    Argument
                      AssignOperator
                        Variable
                        DoubleString
              RParen
```

**tree-sitter CST** `ts=43 nodes`

```
  program
    function_declaration
      identifier
      formal_parameters
      statement_block
        tag_statement
          identifier
          arguments
            assignment_expression
              identifier
              string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  string
        tag_statement
          identifier
          arguments
            assignment_expression
              identifier
              string
            assignment_expression
              identifier
              string
            assignment_expression
              identifier
              string
          statement_block
            expression_statement
              call_expression
                identifier
                arguments
                  assignment_expression
                    identifier
                    string
                  assignment_expression
                    identifier
                    string
                  assignment_expression
                    identifier
                    string
```

### script-corpus / common.txt / 32: common: new with a dotted java path, and the java: prefix

```

a = new java.util.Properties();
b = new cfml.Widget();
c = new java:java.io.File( path );
```

**textparser CST** `tp=47 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              ObjectMember
              Variable
            ObjectMember
            Variable
          LParen
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        PostfixExpressionSuffix
          PostfixExpressionSuffix
            Variable
            ObjectMember
            Variable
          LParen
          RParen
    Semicolon
  ExpressionStatement
    AssignOperator
      Variable
      NewExpression
        NewKeyword
        Variable
  Statement
    Colon
    Variable
    ObjectMember
    Variable
    ObjectMember
    Variable
    LParen
    Variable
    RParen
  Semicolon
```

**tree-sitter CST** `ts=31 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        new_expression
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
    expression_statement
      assignment_expression
        identifier
        new_expression
          member_expression
            identifier
            property_identifier
          arguments
    expression_statement
      assignment_expression
        identifier
        new_expression
          type_prefix
          member_expression
            member_expression
              identifier
              property_identifier
            property_identifier
          arguments
            identifier
```

### script-corpus / real_world.txt / 0: real world: string and array slicing (Lucee LDEV4374)

```

mystring = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
writeOutput( mystring[4:13] );
writeOutput( mystring[4:13:2] );
writeOutput( mystring[-10:-4] );
writeOutput( mystring[-10:-4:2] );
```

**textparser CST** `tp=62 nodes`

```
Repeat
  ExpressionStatement
    AssignOperator
      Variable
      DoubleString
    Semicolon
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    LBracket
    Number
    Colon
    Number
    RBracket
    RParen
  Semicolon
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    LBracket
    Number
    Colon
    Number
    Colon
    Number
    RBracket
    RParen
  Semicolon
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    LBracket
    AddOperator
    Number
    Colon
    AddOperator
    Number
    RBracket
    RParen
  Semicolon
  ExpressionStatement
    Variable
  Statement
    LParen
    Variable
    LBracket
    AddOperator
    Number
    Colon
    AddOperator
    Number
    Colon
    Number
    RBracket
    RParen
  Semicolon
```

**tree-sitter CST** `ts=51 nodes`

```
  program
    expression_statement
      assignment_expression
        identifier
        string
    expression_statement
      call_expression
        identifier
        arguments
          subscript_expression
            identifier
            slice_expression
              number
              number
    expression_statement
      call_expression
        identifier
        arguments
          subscript_expression
            identifier
            slice_expression
              number
              number
              number
    expression_statement
      call_expression
        identifier
        arguments
          subscript_expression
            identifier
            slice_expression
              unary_expression
                unary_operator
                number
              unary_expression
                unary_operator
                number
    expression_statement
      call_expression
        identifier
        arguments
          subscript_expression
            identifier
            slice_expression
              unary_expression
                unary_operator
                number
              unary_expression
                unary_operator
                number
              number
```

### query-corpus / cfquery.txt / 4: malformed select (fault tolerant)

```
SELECT FROM
```

**textparser CST** `tp=5 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    query_keyword
    query_keyword
```

### query-corpus / cfquery.txt / 7: insert with values and hash param

```
INSERT INTO items (a, b) VALUES (1, #ARGUMENTS.x#)
```

**textparser CST** `tp=37 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Variable
        Repeat
          Sequence
            Separator
            Argument
              Variable
      RParen
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
        Repeat
          Sequence
            Separator
            Argument
              SharpExpression
                SharpExpression_Start
                PostfixExpressionSuffix
                  Variable
                  ObjectMember
                  Variable
                SharpExpression_Start
      RParen
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    parenthesized_query_node
      query_identifier
      query_comma
      query_identifier
    query_keyword
    parenthesized_query_node
      query_number
        number
      query_comma
      hash_expression
        member_expression
          identifier
          property_identifier
```

### query-corpus / cfquery.txt / 8: update set and where with hash params

```
UPDATE users SET name = #name# WHERE id = #id#
```

**textparser CST** `tp=23 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      SharpExpression
        SharpExpression_Start
        Variable
        SharpExpression_Start
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      hash_expression
        identifier
    query_keyword
    query_assignment_expression
      query_identifier
      hash_expression
        identifier
```

### query-corpus / cfquery.txt / 9: delete from with hash in where

```
DELETE FROM logs WHERE id = #request.id#
```

**textparser CST** `tp=19 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        SharpExpression_Start
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      hash_expression
        member_expression
          identifier
          property_identifier
```

### query-corpus / cfquery.txt / 10: select inner join on

```
SELECT u.id FROM users u INNER JOIN orders o ON u.id = o.user_id
```

**textparser CST** `tp=34 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
```

**tree-sitter CST** `ts=20 nodes`

```
  program
    query_keyword
    query_alias
      query_identifier
      query_identifier
    query_keyword
    query_identifier
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        query_alias
          query_identifier
          query_identifier
```

### query-corpus / cfquery.txt / 11: select order by desc

```
SELECT * FROM t ORDER BY created_at DESC
```

**textparser CST** `tp=15 nodes`

```
Template
  ExpressionStatement
    MulOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=10 nodes`

```
  program
    query_keyword
    query_star
      star
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_keyword
```

### query-corpus / cfquery.txt / 12: select group by two columns

```
SELECT a, b FROM t GROUP BY a, b
```

**textparser CST** `tp=21 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    query_keyword
    query_identifier
    query_comma
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_comma
    query_identifier
```

### query-corpus / cfquery.txt / 13: where and with parenthesized or

```
SELECT * FROM t WHERE a = 1 AND (b = 2 OR c = 3)
```

**textparser CST** `tp=24 nodes`

```
Template
  ExpressionStatement
    MulOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      LogicalAndOperator
        Number
        ParenthesizedExpression
          LParen
          AssignOperator
            Variable
            AssignOperator
              LogicalOrOperator
                Number
                Variable
              Number
          RParen
```

**tree-sitter CST** `ts=22 nodes`

```
  program
    query_keyword
    query_star
      star
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_number
        number
    query_keyword
    parenthesized_query_node
      query_assignment_expression
        query_identifier
        query_number
          number
      query_keyword
      query_assignment_expression
        query_identifier
        query_number
          number
```

### query-corpus / cfquery.txt / 15: hash param in select list and alias

```
SELECT #COLUMN_NAME# AS col FROM my_table
```

**textparser CST** `tp=15 nodes`

```
Template
  ExpressionStatement
    Variable
  SharpExpression
    SharpExpression_Start
    Variable
    SharpExpression_Start
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=8 nodes`

```
  program
    query_keyword
    hash_expression
      identifier
    query_keyword
    query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 16: sql line comment before select

```
-- active users only
SELECT id FROM users
```

**textparser CST** `tp=16 nodes`

```
Template
  IncDecOperator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    comment
    query_keyword
    query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 17: with cte select

```
WITH active AS (SELECT id FROM users WHERE status = 1)
SELECT * FROM active
```

**textparser CST** `tp=29 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      Number
  RParen
  ExpressionStatement
    MulOperator
      Variable
      Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=19 nodes`

```
  program
    query_keyword
    query_identifier
    query_keyword
    parenthesized_query_node
      query_keyword
      query_identifier
      query_keyword
      query_identifier
      query_keyword
      query_assignment_expression
        query_identifier
        query_number
          number
    query_keyword
    query_star
      star
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 18: truncate table

```
TRUNCATE TABLE temp_logs
```

**textparser CST** `tp=7 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=4 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 20: cast as type

```
SELECT CAST(id AS VARCHAR(10)) FROM t
```

**textparser CST** `tp=23 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
      RParen
  RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=14 nodes`

```
  program
    query_keyword
    query_function
      query_function_name
      parenthesized_query_node
        query_identifier
        query_keyword
        query_function
          query_function_name
          parenthesized_query_node
            query_number
              number
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 21: bracket quoted identifiers

```
SELECT [col] FROM [tbl]
```

**textparser CST** `tp=13 nodes`

```
Template
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LBracket
      Variable
      RBracket
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LBracket
      Variable
      RBracket
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    query_keyword
    bracketed_query_value
      query_value
    query_keyword
    bracketed_query_value
      query_value
```

### query-corpus / cfquery.txt / 24: merge matched update

```
MERGE INTO users AS u USING staging AS s ON u.id = s.id WHEN MATCHED THEN UPDATE SET name = s.name
```

**textparser CST** `tp=48 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
```

**tree-sitter CST** `ts=28 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        query_alias
          query_identifier
          query_identifier
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    query_assignment_expression
      query_identifier
      query_alias
        query_identifier
        query_identifier
```

### query-corpus / cfquery.txt / 25: exec stored procedure

```
EXEC dbo.refresh_cache
```

**textparser CST** `tp=8 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
```

**tree-sitter CST** `ts=5 nodes`

```
  program
    query_keyword
    query_alias
      query_identifier
      query_identifier
```

### query-corpus / cfquery.txt / 26: window clause

```
SELECT 1 FROM t WINDOW w AS (PARTITION BY id ORDER BY sort_key)
```

**textparser CST** `tp=28 nodes`

```
Template
  ExpressionStatement
    Variable
  Number
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  RParen
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    query_keyword
    query_number
      number
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    parenthesized_query_node
      query_keyword
      query_keyword
      query_identifier
      query_keyword
      query_keyword
      query_identifier
```

### query-corpus / cfquery.txt / 29: boolean literals

```
SELECT TRUE, FALSE FROM t WHERE flag = UNKNOWN
```

**textparser CST** `tp=16 nodes`

```
Template
  ExpressionStatement
    Variable
  Boolean
  Separator
  Boolean
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      Variable
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    query_keyword
    query_identifier
    query_comma
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_identifier
```

### query-corpus / cfquery.txt / 30: double quoted identifier

```
SELECT "quoted_col" FROM my_table
```

**textparser CST** `tp=8 nodes`

```
Template
  ExpressionStatement
    Variable
  DoubleString
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=5 nodes`

```
  program
    query_keyword
    double_quoted_query_value
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 32: window function over

```
SELECT ROW_NUMBER() OVER (PARTITION BY id ORDER BY sort_key) FROM t
```

**textparser CST** `tp=28 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      RParen
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    query_keyword
    query_function
      query_function_name
      parenthesized_query_node
    query_keyword
    parenthesized_query_node
      query_keyword
      query_keyword
      query_identifier
      query_keyword
      query_keyword
      query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 33: count star over

```
SELECT COUNT(*) OVER (PARTITION BY gid) FROM t
```

**textparser CST** `tp=22 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  MulOperator
  RParen
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=14 nodes`

```
  program
    query_keyword
    query_function
      query_function_name
      parenthesized_query_node
        query_star
          star
    query_keyword
    parenthesized_query_node
      query_keyword
      query_keyword
      query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 34: merge when not matched insert

```
MERGE INTO t USING s ON t.id = s.id WHEN MATCHED THEN UPDATE SET x = 1 WHEN NOT MATCHED THEN INSERT (a) VALUES (1)
```

**textparser CST** `tp=60 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      Number
  ExpressionStatement
    Variable
  LogicalNotOperator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Variable
      RParen
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
      RParen
```

**tree-sitter CST** `ts=34 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        query_alias
          query_identifier
          query_identifier
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    query_assignment_expression
      query_identifier
      query_number
        number
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    query_keyword
    parenthesized_query_node
      query_identifier
    query_keyword
    parenthesized_query_node
      query_number
        number
```

### query-corpus / cfquery.txt / 37: select with math expression

```
SELECT s.test+1 as test
FROM test s
```

**textparser CST** `tp=20 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    AddOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      Number
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    query_keyword
    query_alias
      query_identifier
      query_concat_expression
        query_identifier
    query_number
      number
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_identifier
```

### query-corpus / cfquery.txt / 43: cfreturn inside cfif

```
SELECT col1 FROM test
<cfif Len(x) LTE 0>
<cfreturn result />
</cfif>
```

**textparser CST** `tp=29 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  IfTagPair
    IfStartTag_Start
    CompareOperator
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            Variable
        RParen
      Number
    TagEnd
    Repeat
      SelfClosingTag
        StartTag_Start
        Repeat
          TagAttribute
            Variable
        TagSelfClose
    IfEndTag
```

**tree-sitter CST** `ts=16 nodes`

```
  program
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    cf_if_tag
      binary_expression
        call_expression
          identifier
          arguments
            identifier
        number
      cf_return_tag
        identifier
        cf_selfclose_void_tag_end
          self_closing_tag_delimiter
```

### query-corpus / cfquery.txt / 45: backtick quoted identifiers

```
SELECT `column_name` FROM `table_name` WHERE `id` = 1
```

**textparser CST** `tp=3 nodes`

```
Template
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    query_keyword
    backtick_quoted_query_value
      query_value
    query_keyword
    backtick_quoted_query_value
      query_value
    query_keyword
    query_assignment_expression
      backtick_quoted_query_value
        query_value
      query_number
        number
```

### query-corpus / cfquery.txt / 46: tsql at variables

```
exec(@script)
while @@fetch_status = 0
BEGIN
SELECT @name
END
```

**textparser CST** `tp=4 nodes`

```
Template
  ExpressionStatement
    Variable
  LParen
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    query_keyword
    parenthesized_query_node
      query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_number
        number
    query_keyword
    query_keyword
    query_identifier
    query_keyword
```

### query-corpus / cfquery.txt / 47: double dash inside quoted string

```
SELECT 'hello -- world' FROM test
-- actual comment
WHERE name = '--value--'
```

**textparser CST** `tp=19 nodes`

```
Template
  ExpressionStatement
    Variable
  SingleString
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  IncDecOperator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      SingleString
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    query_keyword
    quoted_query_value
      query_value
    query_keyword
    query_identifier
    comment
    query_keyword
    query_assignment_expression
      query_identifier
      quoted_query_value
        query_value
```

### query-corpus / cfquery.txt / 51: select with math expression

```
SELECT col1 - col2 / col3 FROM test
```

**textparser CST** `tp=13 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    AddOperator
      Variable
      MulOperator
        Variable
        Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    query_keyword
    query_math_expression
      query_math_expression
        query_identifier
        query_identifier
      query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 52: identifiers beginning with a sql keyword

```
SELECT orders, notes, settings, topic, forms FROM assets
```

**textparser CST** `tp=21 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=13 nodes`

```
  program
    query_keyword
    query_identifier
    query_comma
    query_identifier
    query_comma
    query_identifier
    query_comma
    query_identifier
    query_comma
    query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 53: more identifiers beginning with a sql keyword

```
SELECT selected_items, inner_join, ontario, byline FROM allocation
```

**textparser CST** `tp=18 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    query_keyword
    query_identifier
    query_comma
    query_identifier
    query_comma
    query_identifier
    query_comma
    query_identifier
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 54: exact keyword as identifier needs escaping

```
SELECT [end], "end" FROM t
```

**textparser CST** `tp=13 nodes`

```
Template
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LBracket
      Variable
      RBracket
  Separator
  DoubleString
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=8 nodes`

```
  program
    query_keyword
    bracketed_query_value
      query_value
    query_comma
    double_quoted_query_value
    query_keyword
    query_identifier
```

### query-corpus / cfquery.txt / 55: sql true false null stay query identifiers

```
SELECT * FROM t WHERE a IS NOT NULL AND b = TRUE AND c = FALSE
```

**textparser CST** `tp=21 nodes`

```
Template
  ExpressionStatement
    MulOperator
      Variable
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      LogicalAndOperator
        CompareOperator
          Variable
          NullKeyword
        Variable
      AssignOperator
        LogicalAndOperator
          Boolean
          Variable
        Boolean
```

**tree-sitter CST** `ts=19 nodes`

```
  program
    query_keyword
    query_star
      star
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_identifier
```

### query-corpus / common.txt / 0: common: insert

```
INSERT INTO t (a, b) VALUES (1, 2)
```

**textparser CST** `tp=31 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Variable
        Repeat
          Sequence
            Separator
            Argument
              Variable
      RParen
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
        Repeat
          Sequence
            Separator
            Argument
              Number
      RParen
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    parenthesized_query_node
      query_identifier
      query_comma
      query_identifier
    query_keyword
    parenthesized_query_node
      query_number
        number
      query_comma
      query_number
        number
```

### query-corpus / common.txt / 2: common: delete with where

```
DELETE FROM t WHERE id = 3
```

**textparser CST** `tp=13 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      Variable
      Number
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_assignment_expression
      query_identifier
      query_number
        number
```

### query-corpus / common.txt / 4: common: left outer join with coalesce

```
SELECT COALESCE(a.name, b.name) AS n
FROM a
LEFT OUTER JOIN b ON a.id = b.id
ORDER BY n
```

**textparser CST** `tp=56 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          PostfixExpressionSuffix
            Variable
            ObjectMember
            Variable
        Repeat
          Sequence
            Separator
            Argument
              PostfixExpressionSuffix
                Variable
                ObjectMember
                Variable
      RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=31 nodes`

```
  program
    query_keyword
    query_function
      query_function_name
      parenthesized_query_node
        query_alias
          query_identifier
          query_identifier
        query_comma
        query_alias
          query_identifier
          query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        query_alias
          query_identifier
          query_identifier
    query_keyword
    query_keyword
    query_identifier
```

### query-corpus / common.txt / 6: common: top with multi-column order by

```
SELECT TOP 10 a FROM t ORDER BY a DESC, b ASC
```

**textparser CST** `tp=25 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Number
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    query_keyword
    query_keyword
    query_number
      number
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_comma
    query_identifier
    query_keyword
```

### query-corpus / common.txt / 7: common: cast, isnull and dateadd

```
SELECT CAST(a AS varchar(10)), ISNULL(b, 0), DATEADD(day, 1, c) FROM t
```

**textparser CST** `tp=55 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Number
      RParen
  RParen
  Separator
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Variable
        Repeat
          Sequence
            Separator
            Argument
              Number
      RParen
  Separator
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      LParen
      ArgumentList
        Argument
          Variable
        Repeat
          Sequence
            Separator
            Argument
              Number
          Sequence
            Separator
            Argument
              Variable
      RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=32 nodes`

```
  program
    query_keyword
    query_function
      query_function_name
      parenthesized_query_node
        query_identifier
        query_keyword
        query_function
          query_function_name
          parenthesized_query_node
            query_number
              number
    query_comma
    query_function
      query_function_name
      parenthesized_query_node
        query_identifier
        query_comma
        query_number
          number
    query_comma
    query_function
      query_function_name
      parenthesized_query_node
        query_function_name
        query_comma
        query_number
          number
        query_comma
        query_identifier
    query_keyword
    query_identifier
```

### query-corpus / common.txt / 8: common: union all

```
SELECT a FROM t UNION ALL SELECT b FROM u
```

**textparser CST** `tp=21 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    query_keyword
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_identifier
```

### query-corpus / common.txt / 10: common: group by with having and ordered aggregate

```
SELECT a, COUNT(*) c FROM t GROUP BY a HAVING COUNT(*) > 1 ORDER BY c DESC
```

**textparser CST** `tp=40 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  Separator
  ExpressionStatement
    Variable
  LParen
  MulOperator
  RParen
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  MulOperator
  RParen
  CompareOperator
  Number
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
```

**tree-sitter CST** `ts=28 nodes`

```
  program
    query_keyword
    query_identifier
    query_comma
    query_function
      query_function_name
      parenthesized_query_node
        query_star
          star
    query_identifier
    query_keyword
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_keyword
    query_comparison_expression
      query_function
        query_function_name
        parenthesized_query_node
          query_star
            star
      query_number
        number
    query_keyword
    query_keyword
    query_identifier
    query_keyword
```

### query-corpus / real_world.txt / 2: real world: hash expression in a join predicate (Mura, Slatwall)

```
SELECT c.contentid, c.title
FROM tcontent c
INNER JOIN tcontentstats s ON c.contentid = s.contentid
WHERE c.siteid = #arguments.siteid#
GROUP BY c.contentid, c.title
HAVING count(*) > 1
ORDER BY c.title DESC
```

**textparser CST** `tp=90 nodes`

```
Template
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  Separator
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    AssignOperator
      PostfixExpressionSuffix
        Variable
        ObjectMember
        Variable
      SharpExpression
        SharpExpression_Start
        PostfixExpressionSuffix
          Variable
          ObjectMember
          Variable
        SharpExpression_Start
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  Separator
  ExpressionStatement
    PostfixExpressionSuffix
      Variable
      ObjectMember
      Variable
  ExpressionStatement
    Variable
  ExpressionStatement
    Variable
  LParen
  MulOperator
  RParen
  CompareOperator
  Number
  ExpressionStatement
```

**tree-sitter CST** `ts=57 nodes`

```
  program
    query_keyword
    query_alias
      query_identifier
      query_identifier
    query_comma
    query_alias
      query_identifier
      query_identifier
    query_keyword
    query_identifier
    query_identifier
    query_keyword
    query_keyword
    query_identifier
    query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        query_alias
          query_identifier
          query_identifier
    query_keyword
    query_alias
      query_identifier
      query_assignment_expression
        query_identifier
        hash_expression
          member_expression
            identifier
            property_identifier
    query_keyword
    query_keyword
    query_alias
      query_identifier
      query_identifier
    query_comma
    query_alias
      query_identifier
      query_identifier
    query_keyword
    query_comparison_expression
      query_function
        query_function_name
        parenthesized_query_node
          query_star
            star
      query_number
        number
    query_keyword
    query_keyword
    query_alias
      query_identifier
      query_identifier
    query_keyword
```

### examples / deeply-nested-custom.cfm / deeply-nested-custom.cfm

```
<div>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
<xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz><xyz>
</div>

```

**textparser CST** `tp=3605 nodes`

```
Template
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
```

**tree-sitter CST** `ts=4806 nodes`

```
  program
    element
      start_tag
        tag_name
      element
        start_tag
          tag_name
        element
          start_tag
            tag_name
          element
            start_tag
              tag_name
            element
              start_tag
                tag_name
              element
                start_tag
                  tag_name
                element
                  start_tag
                    tag_name
                  element
                    start_tag
                      tag_name
                    element
                      start_tag
                        tag_name
                      element
                        start_tag
                          tag_name
                        element
                          start_tag
                            tag_name
                          element
                            start_tag
                              tag_name
                            element
                              start_tag
                                tag_name
                              element
                                start_tag
                                  tag_name
                                element
                                  start_tag
                                    tag_name
                                  element
                                    start_tag
                                      tag_name
                                    element
                                      start_tag
                                        tag_name
                                      element
                                        start_tag
                                          tag_name
                                        element
                                          start_tag
                                            tag_name
                                          element
                                            start_tag
                                              tag_name
                                            element
                                              start_tag
                                                tag_name
                                              element
                                                start_tag
                                                  tag_name
                                                element
                                                  start_tag
                                                    tag_name
                                                  element
                                                    start_tag
                                                      tag_name
                                                    element
                                                      start_tag
                                                        tag_name
                                                      element
                                                        start_tag
                                                          tag_name
                                                        element
```

### examples / deeply-nested.cfm / deeply-nested.cfm

```
<div>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>
</div>

```

**textparser CST** `tp=5945 nodes`

```
Template
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
    StartTag_Start
    TagEnd
  StartTag
```

**tree-sitter CST** `ts=7926 nodes`

```
  program
    element
      start_tag
        tag_name
      element
        start_tag
          tag_name
        element
          start_tag
            tag_name
          element
            start_tag
              tag_name
            element
              start_tag
                tag_name
              element
                start_tag
                  tag_name
                element
                  start_tag
                    tag_name
                  element
                    start_tag
                      tag_name
                    element
                      start_tag
                        tag_name
                      element
                        start_tag
                          tag_name
                        element
                          start_tag
                            tag_name
                          element
                            start_tag
                              tag_name
                            element
                              start_tag
                                tag_name
                              element
                                start_tag
                                  tag_name
                                element
                                  start_tag
                                    tag_name
                                  element
                                    start_tag
                                      tag_name
                                    element
                                      start_tag
                                        tag_name
                                      element
                                        start_tag
                                          tag_name
                                        element
                                          start_tag
                                            tag_name
                                          element
                                            start_tag
                                              tag_name
                                            element
                                              start_tag
                                                tag_name
                                              element
                                                start_tag
                                                  tag_name
                                                element
                                                  start_tag
                                                    tag_name
                                                  element
                                                    start_tag
                                                      tag_name
                                                    element
                                                      start_tag
                                                        tag_name
                                                      element
                                                        start_tag
                                                          tag_name
                                                        element
```

### tp-unit / cfml_tests.cpp / 1

```
<cfscript>
        /* block comment */
        // line comment
        x = 1;
    </cfscript>
```

**textparser CST** `tp=11 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 9

```
<cfset x = "hello ##name##" />
```

**textparser CST** `tp=7 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      DoubleString
    TagSelfClose
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        string
        #
        #
        #
        #
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
```

### tp-unit / cfml_tests.cpp / 12

```
<cfset x = "#func(arg1, "#nested#")#" />
```

**textparser CST** `tp=7 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      DoubleString
    TagSelfClose
```

**tree-sitter CST** `ts=12 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        string
        #
          ,
            hash_expression
              identifier
        #
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
```

### tp-unit / cfml_tests.cpp / 13

```
<cfscript>
        x = 1;
        y = 2;
    </cfscript>
```

**textparser CST** `tp=16 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 14

```
<cfscript>
        if (x) {
            myStruct = { a = 1, b = 2 };
        }
    </cfscript>
```

**textparser CST** `tp=34 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      IfStatement
        IfKeyword
        LParen
        Variable
        RParen
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              AssignOperator
                Variable
                StructLiteral
                  LBrace
                  StructMemberList
                    StructMember
                      Variable
                      AssignOperator
                      Number
                    Repeat
                      Sequence
                        Separator
                        StructMember
                          Variable
                          AssignOperator
                          Number
                  RBrace
              Semicolon
          RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 15

```
<cfscript>
        x += 1;
        y -= 1;
        z = a && b;
    </cfscript>
```

**textparser CST** `tp=23 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          LogicalAndOperator
            Variable
            Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 16

```
<cfscript>writeOutput(1 && 3);</cfscript>
```

**textparser CST** `tp=17 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              LogicalAndOperator
                Number
                Number
          RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 17

```
<cfscript>writeOutput(0 || 3);</cfscript>
```

**textparser CST** `tp=17 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              LogicalOrOperator
                Number
                Number
          RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 18

```
<cfoutput>#0 || 3#</cfoutput>
```

**textparser CST** `tp=12 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        LogicalOrOperator
          Number
          Number
        SharpExpression_Start
    OutputEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_output_tag
      hash_expression
        binary_expression
          number
          number
```

### tp-unit / cfml_tests.cpp / 20

```
<cfoutput>#"x #a#"#</cfoutput>
```

**textparser CST** `tp=10 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        DoubleString
        SharpExpression_Start
    OutputEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_output_tag
      hash_expression
        string
        #
        #
```

### tp-unit / cfml_tests.cpp / 21

```
<cfoutput>outer <cfoutput>inner #var#</cfoutput> end</cfoutput>
```

**textparser CST** `tp=21 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        Variable
      OutputTagPair
        OutputStartTag_Start
        TagEnd
        Repeat
          ExpressionStatement
            Variable
          SharpExpression
            SharpExpression_Start
            VarKeyword
            SharpExpression_Start
        OutputEndTag
      ExpressionStatement
        Variable
    OutputEndTag
```

**tree-sitter CST** `ts=8 nodes`

```
  program
    cf_output_tag
      html_text
      cf_output_tag
        html_text
        hash_expression
          identifier
      html_text
```

### tp-unit / cfml_tests.cpp / 23

```
<cfquery name="q">SELECT * FROM t WHERE id = #id#</cfquery>
```

**textparser CST** `tp=27 nodes`

```
Template
  QueryTagPair
    QueryStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        MulOperator
          Variable
          Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        Variable
      ExpressionStatement
        AssignOperator
          Variable
          SharpExpression
            SharpExpression_Start
            Variable
            SharpExpression_Start
    QueryEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_query_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_query_content
```

### tp-unit / cfml_tests.cpp / 31

```
<cfset x = func1(func2(a, b), c) />
```

**textparser CST** `tp=29 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      PostfixExpressionSuffix
        Variable
        LParen
        ArgumentList
          Argument
            PostfixExpressionSuffix
              Variable
              LParen
              ArgumentList
                Argument
                  Variable
                Repeat
                  Sequence
                    Separator
                    Argument
                      Variable
              RParen
          Repeat
            Sequence
              Separator
              Argument
                Variable
        RParen
    TagSelfClose
```

**tree-sitter CST** `ts=15 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        call_expression
          identifier
          arguments
            call_expression
              identifier
              arguments
                identifier
                identifier
            identifier
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
```

### tp-unit / cfml_tests.cpp / 38

```
<cfoutput><cfloop from="1" to="2" index="i">#i#</cfloop></cfoutput>
```

**textparser CST** `tp=31 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      LoopTagPair
        LoopStartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
        Repeat
          SharpExpression
            SharpExpression_Start
            Variable
            SharpExpression_Start
        LoopEndTag
    OutputEndTag
```

**tree-sitter CST** `ts=21 nodes`

```
  program
    cf_output_tag
      cf_tag
        cf_start_tag
          cf_tag_name
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
          cf_tag_attributes
            cf_attribute
              cf_attribute_name
              quoted_cf_attribute_value
        hash_expression
          identifier
        cf_end_tag
          cf_tag_name
```

### tp-unit / cfml_tests.cpp / 39

```
<cfquery name="q"><cfloop from="1" to="2" index="i">#i#</cfloop></cfquery>
```

**textparser CST** `tp=37 nodes`

```
Template
  QueryTagPair
    QueryStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      LoopTagPair
        LoopStartTag_Start
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        TagEnd
        Repeat
          SharpExpression
            SharpExpression_Start
            Variable
            SharpExpression_Start
        LoopEndTag
    QueryEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_query_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_query_content
```

### tp-unit / cfml_tests.cpp / 46

```
<cfsavecontent variable="body">Hello</cfsavecontent>
```

**textparser CST** `tp=14 nodes`

```
Template
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
    SavecontentEndTag
```

**tree-sitter CST** `ts=7 nodes`

```
  program
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        html_text
```

### tp-unit / cfml_tests.cpp / 47

```
<cfsavecontent variable="body"><cfoutput>#x#</cfoutput></cfsavecontent>
```

**textparser CST** `tp=21 nodes`

```
Template
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      OutputTagPair
        OutputStartTag_Start
        TagEnd
        Repeat
          SharpExpression
            SharpExpression_Start
            Variable
            SharpExpression_Start
        OutputEndTag
    SavecontentEndTag
```

**tree-sitter CST** `ts=9 nodes`

```
  program
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        cf_output_tag
          hash_expression
            identifier
```

### tp-unit / cfml_tests.cpp / 48

```
<cfmail to="a">x</cfmail><cfsavecontent variable="v">y</cfsavecontent>
```

**textparser CST** `tp=27 nodes`

```
Template
  MailTagPair
    MailStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
    MailEndTag
  SavecontentTagPair
    SavecontentStartTag_Start
    Repeat
      TagAttribute
        Variable
        Sequence
          AssignOperator
          DoubleString
    TagEnd
    Repeat
      ExpressionStatement
        Variable
    SavecontentEndTag
```

**tree-sitter CST** `ts=17 nodes`

```
  program
    cf_tag
      cf_start_tag
        cf_tag_name
        cf_tag_attributes
          cf_attribute
            cf_attribute_name
            quoted_cf_attribute_value
      html_text
      cf_end_tag
        cf_tag_name
    cf_savecontent_tag
      cf_attribute
        cf_attribute_name
        quoted_cf_attribute_value
      cf_savecontent_body
        html_text
```

### tp-unit / cfml_tests.cpp / 67

```
<cfscript>foo(-1);</cfscript>
```

**textparser CST** `tp=16 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          ArgumentList
            Argument
              AddOperator
                Number
          RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 68

```
<cfset arr = [1, -2] />
```

**textparser CST** `tp=18 nodes`

```
Template
  SetTag
    SetStartTag_Start
    AssignOperator
      Variable
      ArrayLiteral
        LBracket
        ArgumentList
          Argument
            Number
          Repeat
            Sequence
              Separator
              Argument
                AddOperator
                  Number
        RBracket
    TagSelfClose
```

**tree-sitter CST** `ts=11 nodes`

```
  program
    cf_set_tag
      assignment_expression
        identifier
        array
          number
          unary_expression
            unary_operator
            number
      cf_selfclose_void_tag_end
        self_closing_tag_delimiter
```

### tp-unit / cfml_tests.cpp / 69

```
<cfoutput>#-1#</cfoutput>
```

**textparser CST** `tp=11 nodes`

```
Template
  OutputTagPair
    OutputStartTag_Start
    TagEnd
    Repeat
      SharpExpression
        SharpExpression_Start
        AddOperator
          Number
        SharpExpression_Start
    OutputEndTag
```

**tree-sitter CST** `ts=6 nodes`

```
  program
    cf_output_tag
      hash_expression
        unary_expression
          unary_operator
          number
```

### tp-unit / cfml_tests.cpp / 70

```
<cfscript>if (1 - -2 == 3) writeOutput("ok");</cfscript>
```

**textparser CST** `tp=25 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      IfStatement
        IfKeyword
        LParen
        CompareOperator
          AddOperator
            Number
            AddOperator
              Number
          Number
        RParen
        ExpressionStatement
          PostfixExpressionSuffix
            Variable
            LParen
            ArgumentList
              Argument
                DoubleString
            RParen
          Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 71

```
<cfscript>res = user?.profile?.name;</cfscript>
```

**textparser CST** `tp=17 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          PostfixExpressionSuffix
            PostfixExpressionSuffix
              Variable
              SafeNavigation
              Variable
            SafeNavigation
            Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 72

```
<cfscript>res = missingVar ?? "Fallback";</cfscript>
```

**textparser CST** `tp=13 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          NullCoalescingOperator
            Variable
            DoubleString
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 73

```
<cfscript>a = (1 === 1); b = (1 !== "1");</cfscript>
```

**textparser CST** `tp=26 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          ParenthesizedExpression
            LParen
            CompareOperator
              Number
              Number
            RParen
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          ParenthesizedExpression
            LParen
            CompareOperator
              Number
              DoubleString
            RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 74

```
<cfscript>arr2 = [1, ...arr1, 4];</cfscript>
```

**textparser CST** `tp=26 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          ArrayLiteral
            LBracket
            ArgumentList
              Argument
                Number
              Repeat
                Sequence
                  Separator
                  Argument
                    SpreadOperator
                    Variable
                Sequence
                  Separator
                  Argument
                    Number
            RBracket
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 75

```
<cfscript>x++; ++x; y--; --y;</cfscript>
```

**textparser CST** `tp=22 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 76

```
<cfscript>x++; y--;</cfscript>
```

**textparser CST** `tp=14 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 77

```
<cfscript>++p; --q;</cfscript>
```

**textparser CST** `tp=14 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
      ExpressionStatement
        IncDecOperator
          Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 78

```
<cfscript>a + b - c;</cfscript>
```

**textparser CST** `tp=13 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AddOperator
          AddOperator
            Variable
            Variable
          Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 79

```
<cfscript>a + + b;</cfscript>
```

**textparser CST** `tp=12 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AddOperator
          Variable
          AddOperator
            Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 80

```
<cfscript>x++ + y;</cfscript>
```

**textparser CST** `tp=12 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AddOperator
          IncDecOperator
            Variable
          Variable
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 81

```
<cfscript>arr[++n];</cfscript>
```

**textparser CST** `tp=14 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LBracket
          IncDecOperator
            Variable
          RBracket
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 83

```
<cfscript>i += 1; i -= 1;</cfscript>
```

**textparser CST** `tp=16 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 84

```
<cfscript>switch(val) { case "B": res = "Got B"; break; default: continue; }</cfscript>
```

**textparser CST** `tp=34 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      SwitchStatement
        SwitchKeyword
        LParen
        Variable
        RParen
        LBrace
        Repeat
          SwitchCase
            CaseKeyword
            DoubleString
            Colon
            Repeat
              ExpressionStatement
                AssignOperator
                  Variable
                  DoubleString
                Semicolon
              BreakStatement
                BreakKeyword
                Semicolon
          SwitchCase
            DefaultKeyword
            Colon
            Repeat
              ContinueStatement
                ContinueKeyword
                Semicolon
        RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 85

```
<cfscript>for (i in list) { while(c < 3) { c++; } do { d++; } while (d < 3); }</cfscript>
```

**textparser CST** `tp=49 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ForInStatement
        ForKeyword
        LParen
        Variable
        InKeyword
        Variable
        RParen
        BlockStatement
          LBrace
          Repeat
            WhileStatement
              WhileKeyword
              LParen
              CompareOperator
                Variable
                Number
              RParen
              BlockStatement
                LBrace
                Repeat
                  ExpressionStatement
                    IncDecOperator
                      Variable
                    Semicolon
                RBrace
            DoWhileStatement
              DoKeyword
              BlockStatement
                LBrace
                Repeat
                  ExpressionStatement
                    IncDecOperator
                      Variable
                    Semicolon
                RBrace
              WhileKeyword
              LParen
              CompareOperator
                Variable
                Number
              RParen
              Semicolon
          RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 86

```
<cfscript>try { throw("E"); } catch (any e) { rethrow; retry; } finally { return; }</cfscript>
```

**textparser CST** `tp=45 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      TryCatchStatement
        TryKeyword
        BlockStatement
          LBrace
          Repeat
            ThrowStatement
              ThrowKeyword
              ParenthesizedExpression
                LParen
                DoubleString
                RParen
              Semicolon
          RBrace
        Repeat
          CatchClause
            CatchKeyword
            LParen
            Variable
            Variable
            RParen
            BlockStatement
              LBrace
              Repeat
                RethrowStatement
                  RethrowKeyword
                  Semicolon
                RetryStatement
                  RetryKeyword
                  Semicolon
              RBrace
        Sequence
          FinallyKeyword
          BlockStatement
            LBrace
            Repeat
              ReturnStatement
                ReturnKeyword
                Semicolon
            RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 87

```
<cfscript>import java.util.*; pageencoding "UTF-8"; include "helper.cfm"; public static final function test() {}</cfscript>
```

**textparser CST** `tp=38 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ImportStatement
        ImportKeyword
        QualifiedIdentifier
          Variable
          Repeat
            Sequence
              ObjectMember
              Variable
            Sequence
              ObjectMember
              MulOperator
        Semicolon
      PageencodingStatement
        PageencodingKeyword
        DoubleString
        Semicolon
      IncludeStatement
        IncludeKeyword
        DoubleString
        Semicolon
      FunctionDeclaration
        Repeat
          PublicKeyword
          StaticKeyword
          FinalKeyword
        FunctionKeyword
        Variable
        LParen
        RParen
        BlockStatement
          LBrace
          RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 88

```
<cfscript>x = new com.foo.Bar();</cfscript>
```

**textparser CST** `tp=22 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          NewExpression
            NewKeyword
            PostfixExpressionSuffix
              PostfixExpressionSuffix
                PostfixExpressionSuffix
                  Variable
                  ObjectMember
                  Variable
                ObjectMember
                Variable
              LParen
              RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 90

```
<cfscript>a = new b();</cfscript>
```

**textparser CST** `tp=16 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          NewExpression
            NewKeyword
            PostfixExpressionSuffix
              Variable
              LParen
              RParen
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 93

```
<cfscript>newer = 1; renew(); mynew = 2; newStuf = 3;</cfscript>
```

**textparser CST** `tp=27 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        PostfixExpressionSuffix
          Variable
          LParen
          RParen
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          Number
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 94

```
<cfscript>property name="id" required="true"; param name="p" default="1"; lock name="L" { super.init(); null; } transaction { thread name="T" {} }</cfscript>
```

**textparser CST** `tp=75 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      PropertyDeclaration
        PropertyKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            RequiredKeyword
            Sequence
              AssignOperator
              DoubleString
        Semicolon
      ParamStatement
        ParamKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
          TagAttribute
            DefaultKeyword
            Sequence
              AssignOperator
              DoubleString
        Semicolon
      LockStatement
        LockKeyword
        Repeat
          TagAttribute
            Variable
            Sequence
              AssignOperator
              DoubleString
        BlockStatement
          LBrace
          Repeat
            ExpressionStatement
              PostfixExpressionSuffix
                PostfixExpressionSuffix
                  SuperKeyword
                  ObjectMember
                  Variable
                LParen
                RParen
              Semicolon
            ExpressionStatement
              NullKeyword
              Semicolon
          RBrace
      TransactionStatement
        TransactionKeyword
        BlockStatement
          LBrace
          Repeat
            ThreadStatement
              ThreadKeyword
              Repeat
                TagAttribute
                  Variable
                  Sequence
                    AssignOperator
                    DoubleString
              BlockStatement
                LBrace
                RBrace
          RBrace
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 95

```
<cfscript>doubleFn = x => x * 2; addFn = (a, b) => a + b; blockFn = (x, y) => { return x * y; };</cfscript>
```

**textparser CST** `tp=63 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          ArrowFunction
            Variable
            LambdaOperator
            MulOperator
              Variable
              Number
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          ArrowFunction
            Sequence
              LParen
              ParameterList
                Parameter
                  Variable
                Repeat
                  Sequence
                    Separator
                    Parameter
                      Variable
              RParen
            LambdaOperator
            AddOperator
              Variable
              Variable
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          ArrowFunction
            Sequence
              LParen
              ParameterList
                Parameter
                  Variable
                Repeat
                  Sequence
                    Separator
                    Parameter
                      Variable
              RParen
            LambdaOperator
            BlockStatement
              LBrace
              Repeat
                ReturnStatement
                  ReturnKeyword
                  MulOperator
                    Variable
                    Variable
                  Semicolon
              RBrace
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 96

```
<cfscript>[first, second, ...rest] = [100, 200, 300, 400];</cfscript>
```

**textparser CST** `tp=44 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          ArrayLiteral
            LBracket
            ArgumentList
              Argument
                Variable
              Repeat
                Sequence
                  Separator
                  Argument
                    Variable
                Sequence
                  Separator
                  Argument
                    SpreadOperator
                    Variable
            RBracket
          ArrayLiteral
            LBracket
            ArgumentList
              Argument
                Number
              Repeat
                Sequence
                  Separator
                  Argument
                    Number
                Sequence
                  Separator
                  Argument
                    Number
                Sequence
                  Separator
                  Argument
                    Number
            RBracket
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

### tp-unit / cfml_tests.cpp / 97

```
<cfscript>s1 = { "a": 1, b: 2 }; s2 = { "a" = 1, b = 2 }; ord = [ a = 1, b = 2, c = 3 ];</cfscript>
```

**textparser CST** `tp=69 nodes`

```
Template
  ScriptTagPair
    ScriptStartTag_Start
    TagEnd
    Repeat
      ExpressionStatement
        AssignOperator
          Variable
          StructLiteral
            LBrace
            StructMemberList
              StructMember
                DoubleString
                Colon
                Number
              Repeat
                Sequence
                  Separator
                  StructMember
                    Variable
                    Colon
                    Number
            RBrace
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          StructLiteral
            LBrace
            StructMemberList
              StructMember
                DoubleString
                AssignOperator
                Number
              Repeat
                Sequence
                  Separator
                  StructMember
                    Variable
                    AssignOperator
                    Number
            RBrace
        Semicolon
      ExpressionStatement
        AssignOperator
          Variable
          ArrayLiteral
            LBracket
            ArgumentList
              Argument
                AssignOperator
                  Variable
                  Number
              Repeat
                Sequence
                  Separator
                  Argument
                    AssignOperator
                      Variable
                      Number
                Sequence
                  Separator
                  Argument
                    AssignOperator
                      Variable
                      Number
            RBracket
        Semicolon
    ScriptEndTag
```

**tree-sitter CST** `ts=3 nodes`

```
  program
    cf_script_tag
      cf_script_content
```

