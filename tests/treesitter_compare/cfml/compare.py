#!/usr/bin/env python3
"""Differential CST verification of the textparser CFML grammar against
tree-sitter-cfml (cfml + cfscript grammars, as used by the zed-cfml extension).

Run from the repository root after tests/treesitter_compare/cfml/build.sh:
    python3 tests/treesitter_compare/cfml/compare.py

Emits into tests/treesitter_compare/cfml/:
  fixtures_report.md  - acceptance parity over a CFML construct corpus plus a
                        node-kind census for each parser
  constructs_report.md- node-kind + tree-shape comparison for each construct
                        (textparser CST vs tree-sitter outline)
  work/               - per-file raw outputs used to build the reports
"""
import json, os, subprocess, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))   # repo root
CSTD = os.path.join(HERE, "cstdump")
DUMP = os.path.join(HERE, "dump")
FIX = os.path.join(HERE, "fixtures")
WORK = os.path.join(HERE, "work")
os.makedirs(WORK, exist_ok=True)
os.makedirs(FIX, exist_ok=True)

# ------------------------------------------------------------- corpus --------
# One self-contained CFML construct each, with a name. When a construct needs
# script syntax it is wrapped in <cfscript> (textparser parses script only
# inside a ScriptTagPair; this is also where the zed extension parses script
# with the cfscript grammar).
CONSTRUCTS = {
    # ---- plain tags ----
    "set_basic": "<cfset a = 1234 />\n",
    "set_expr": "<cfset x = a + b * (c - d) % e />\n",
    "set_ternary": "<cfset t = cond ? x : y />\n",
    "set_elvis": "<cfset t = maybe ?: fallback />\n",
    "set_nullcoalesce": "<cfset t = maybe ?? fallback />\n",
    "set_power": "<cfset r = 2 ^ 3 * 2 />\n",
    "set_intdiv": "<cfset r = 8 \\ 3 * 2 />\n",
    "set_mod_word": "<cfset r = 8 MOD 3 * 2 />\n",
    "set_concat": "<cfset s = 'A' & 2 + 3 />\n",
    "set_compare_word": "<cfset b = 1 EQ '1' />\n",
    "set_compare_symbols": "<cfset b = (a >= 1) && (a <= 10) />\n",
    "set_contains": "<cfset b = 'hello' CONTAINS 'ell' />\n",
    "set_does_not_contain": "<cfset b = 'hello' DOES NOT CONTAIN 'xyz' />\n",
    "set_is": "<cfset b = x IS 5 />\n",
    "set_negation": "<cfset b = NOT a AND b OR c />\n",
    "set_imp_eqv_xor": "<cfset b = false IMP true EQV false XOR true />\n",
    "set_compound_assign": "<cfset x += 1 /><cfset y -= 2 /><cfset z &= 'end' />\n",

    # ---- operator-precedence battery (harmonised with CFML_Precedence.md;
    # every case below was evaluated on a real CF/ Lucee server) ----
    "set_pow_leftassoc": "<cfset r = 2 ^ 3 ^ 2 />\n",              # = (2^3)^2 = 64
    "set_pow_unary": "<cfset r = -2 ^ 2 />\n",                     # = (-2)^2 = 4
    "set_intdiv_mod": "<cfset r = 8 MOD 5 \\ 2 />\n",              # = 8 MOD (5\2) = 0
    "set_concat_eq": "<cfset r = 'a' & 'b' EQ 'ab' />\n",          # = ('a' & 'b') EQ 'ab' = YES
    "set_not_compare": "<cfset r = NOT 0 GT 3 />\n",               # = NOT (0 GT 3) = true
    "set_not_and": "<cfset r = not false and false />\n",          # = (not false) and false = false
    "set_and_or": "<cfset r = true or true and false />\n",        # = true or (true and false) = true
    "set_xor_or": "<cfset r = true xor false or true />\n",        # = true xor (false or true) = NO
    "set_imp_eqv": "<cfset r = false imp false eqv false />\n",    # = false imp (false eqv false) = true
    "set_eq_lt_mixed": "<cfset r = 1 EQ 1 LT 2 />\n",              # all comparisons one level: (1 EQ 1) LT 2
    "set_bang": "<cfset r = !false and true />\n",                 # = (!false) and true
    "set_is_not_word": "<cfset r = x IS NOT y />\n",               # `IS NOT` is a single comparison operator
    "set_triple_eq": "<cfset r = 1 === '1' />\n",                  # `===` comparison operator
    "set_safe_nav": "<cfset v = st?.a?.b />\n",
    "set_struct_literal": "<cfset s = { a = 1, b = 'two', c = [1, 2, 3] } />\n",
    "set_array_literal": "<cfset a = [1, 2, 3] />\n",
    "set_hash_in_string": "<cfset s = \"hello #name# world\" />\n",
    "output_basic": "<cfoutput>Hello #name#</cfoutput>\n",
    "output_hash_expr": "<cfoutput>#a + b#</cfoutput>\n",
    "component_basic": "<cfcomponent>\n  <cffunction name=\"foo\">\n    <cfreturn 1 />\n  </cffunction>\n</cfcomponent>\n",
    "component_extends": "<cfcomponent extends=\"Base\" hint=\"the base\">\n  <cfproperty name=\"x\" type=\"numeric\" />\n</cfcomponent>\n",
    "if_elseif_else": "<cfif x GT 0>\n  <cfset pos = true />\n<cfelseif x LT 0>\n  <cfset neg = true />\n<cfelse>\n  <cfset zero = true />\n</cfif>\n",
    "query_tag": "<cfquery name=\"q\" datasource=\"ds\">SELECT * FROM t WHERE id = <cfqueryparam value=\"#id#\" /></cfquery>\n",
    "loop_index": "<cfloop from=\"1\" to=\"10\" index=\"i\">\n  <cfoutput>#i#</cfoutput>\n</cfloop>\n",
    "loop_array": "<cfloop array=\"#arr#\" index=\"i\">#i#</cfloop>\n",
    "loop_query": "<cfloop query=\"q\">#q.name#</cfloop>\n",
    "switch_tag": "<cfswitch expression=\"#x#\">\n  <cfcase value=\"1\">one</cfcase>\n  <cfdefaultcase>other</cfdefaultcase>\n</cfswitch>\n",
    "mail_tag": "<cfmail to=\"a@b.c\" from=\"d@e.f\" subject=\"hi\">body #x#</cfmail>\n",
    "savecontent": "<cfsavecontent variable=\"buf\">#a# #b#</cfsavecontent>\n",
    "include_tag": "<cfinclude template=\"page.cfm\" />\n",
    "param_tag": "<cfparam name=\"x\" default=\"1\" />\n",
    "comment": "<!--- a comment --->\n<cfset x = 1 />\n",
    "html_mixed": "<html><body><p>Hello</p><cfset y = 2 /></body></html>\n",
    "custom_tag": "<cf_myTag attr=\"1\" >body</cf_myTag>\n",
    "script_tag": "<cfscript>\n  x = 1;\n  y = 2;\n</cfscript>\n",
    "cfoutput_query": "<cfoutput query=\"q\">#q.name#</cfoutput>\n",
    "cflocation": "<cflocation url=\"index.cfm\" addtoken=\"false\" />\n",

    # ---- script content (wrapped; tree-sitter parses the body with cfscript) ----
    "script_var": "<cfscript>var x = 1;</cfscript>\n",
    "script_struct": "<cfscript>s = { a = 1, b = [1,2] };</cfscript>\n",
    "script_function": "<cfscript>function add(a, b) { return a + b; }</cfscript>\n",
    "script_if": "<cfscript>if (a > 1) { b = 2; } else { b = 3; }</cfscript>\n",
    "script_for": "<cfscript>for (i = 1; i <= 10; i++) { sum += i; }</cfscript>\n",
    "script_forin": "<cfscript>for (k in s) { writeOutput(k); }</cfscript>\n",
    "script_while": "<cfscript>while (x < 10) { x++; }</cfscript>\n",
    "script_dowhile": "<cfscript>do { x++; } while (x < 10);</cfscript>\n",
    "script_switch": "<cfscript>switch (x) { case 1: break; default: x = 0; }</cfscript>\n",
    "script_trycatch": "<cfscript>try { risky(); } catch (any e) { log(e.message); }</cfscript>\n",
    "script_throw": "<cfscript>throw new Exception('boom');</cfscript>\n",
    "script_new": "<cfscript>obj = new com.example.Foo();</cfscript>\n",
    "script_arrow": "<cfscript>f = (a, b) => a + b;</cfscript>\n",
    "script_spread": "<cfscript>x = [1, ...more];</cfscript>\n",
    "script_elvis_chain": "<cfscript>v = data.maybe ?: 'fallback';</cfscript>\n",
    "script_operator_prec": "<cfscript>r = 2 ^ 3 * 2 + 1 & 'x' EQ '9x';</cfscript>\n",
    "script_component_decl": "<cfscript>\ncomponent extends=\"Base\" {\n  // member comment\n  property name=\"p\" type=\"string\";\n  function init() { return this; }\n}\n</cfscript>\n",
    "script_interface_decl": "<cfscript>\ninterface Marker {\n  function doIt();\n}\n</cfscript>\n",

    # ---- .cfs (script-only file, cfscript grammar) ----
    "cfs_basic": (".cfs", "x = 1;\ny = 2;\n"),
    "cfs_function": (".cfs", "function add(a, b) {\n  return a + b;\n}\n"),
    "cfs_component": (".cfs", "component {\n  property name=\"a\" type=\"numeric\";\n  function getA() { return this.a; }\n}\n"),
    "cfs_import": (".cfs", "import com.foo.Bar;\n"),
    "cfs_binary_ops": (".cfs", "r = a EQ b AND c GT d OR NOT e;\n"),
}

# Script-only constructs: pick grammar by extension in tree-sitter; for
# textparser, .cfs files are not a supported extension, so we wrap them in a
# <cfscript> pair for the textparser run (and note it in the report).
WRAP_FOR_TEXTPARSER = {"cfs_basic", "cfs_function", "cfs_component", "cfs_import", "cfs_binary_ops"}

# ---------------------------------------------------------------- helpers ---

def run(cmd, cwd):
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    return p.stdout, p.stderr

def tp_parse(path):
    out, _ = run([CSTD, path], ROOT)
    try:
        return json.loads(out)
    except Exception:
        return {"status": "BROKEN", "raw": out[:200]}

def ts_parse(path):
    out, err = run([DUMP, path], HERE)
    return out.strip(), "ROOT_ERROR=1" in err

def cst_lines(node, out, depth=0):
    out.append("    " * depth + node.get("kind", "?"))
    for c in node.get("children", []):
        cst_lines(c, out, depth + 1)

def sexp_outline(sexp):
    out = []
    depth = 0
    i, n = 0, len(sexp)
    while i < n:
        c = sexp[i]
        if c == '(':
            depth += 1; i += 1
        elif c == ')':
            depth -= 1; i += 1
        elif c == '"':
            j = i + 1
            while j < n and sexp[j] != '"':
                j += 1
            out.append("    " * depth + '"' + sexp[i + 1:j] + '"')
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and sexp[j] not in '()"' and not sexp[j].isspace():
                j += 1
            out.append("    " * depth + sexp[i:j])
            i = j
    return out

def kind_walk(node, counter):
    counter[node.get("kind", "?")] += 1
    for c in node.get("children", []):
        kind_walk(c, counter)

def sexp_to_tokens(sexp):
    toks = []
    i, n = 0, len(sexp)
    while i < n:
        c = sexp[i]
        if c == '(':
            toks.append('('); i += 1
        elif c == ')':
            toks.append(')'); i += 1
        elif c == '"':
            j = i + 1
            while j < n and sexp[j] != '"':
                j += 1
            toks.append('Q' + sexp[i + 1:j]); i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and sexp[j] not in '()"' and not sexp[j].isspace():
                j += 1
            toks.append(sexp[i:j]); i = j
    return toks

def ts_kinds(toks, counter):
    it = iter(toks)
    def node():
        typ = next(it)
        kids = []
        while True:
            t = next(it)
            if t == '(':
                kids.append(node())
            elif t == ')':
                return (typ, kids)
            elif t.startswith('Q'):
                kids.append(("tok", [], t[1:]))
            else:
                kids.append(("tok", [], t))
    def walk(n):
        counter[n[0]] += 1
        for k in n[1]:
            if k[0] == "tok":
                counter["tok:" + k[2]] += 1
            else:
                walk(k)
    walk(node())

# ------------------------------------------------------------ reports --------

def fixtures_report():
    out = []
    out.append("# Fixture corpus parity: textparser vs tree-sitter-cfml\n")
    out.append("`tp` = textparser (definitions/cfml_definition.json); `ts` = "
               "tree-sitter cfml/cfscript grammar at the rev pinned by the "
               "zed extension. A construct is *rejected* by textparser when its "
               "status is not `OK` or it carries diagnostics; tree-sitter rejects "
               "when its root tree has an ERROR node. `parity` is OK when both "
               "agree (both accept or both reject).\n")
    header = "| construct | tp status | tp diag | tp rejects | ts rejects | parity |"
    out.append(header)
    out.append("|---|---|---|---|---|---|")
    census_tp = collections.Counter()
    census_ts = collections.Counter()
    rows = []
    for name, spec in CONSTRUCTS.items():
        ext, src = spec if isinstance(spec, tuple) else (".cfm", spec)
        path = os.path.join(WORK, name + ext)
        with open(path, "w") as f:
            f.write(src)
        tp_path = path
        wrapped = False
        if name in WRAP_FOR_TEXTPARSER:
            wrapped = True
            tp_path = os.path.join(WORK, name + ".cfm")
            with open(tp_path, "w") as f:
                f.write("<cfscript>\n" + src + "</cfscript>\n")
        tp = tp_parse(tp_path)
        tp_status = tp.get("status")
        diags = tp.get("diagnostics") or []
        tp_rejects = (tp_status != "OK") or (len(diags) > 0)
        sexp, ts_err = ts_parse(path)
        ts_rejects = ts_err
        if tp.get("cst"):
            kind_walk(tp["cst"], census_tp)
        else:
            census_tp["<no-cst>"] += 1
        toks = sexp_to_tokens(sexp)
        ts_kinds(toks, census_ts)
        parity = "OK" if tp_rejects == ts_rejects else "MISMATCH"
        wrapped_note = " (wrapped for tp)" if wrapped else ""
        rows.append((name + wrapped_note, tp_status, len(diags), tp_rejects, ts_rejects, parity))
    for name, st, dc, tpr, tsr, parity in rows:
        out.append(f"| {name} | {st} | {dc} | {tpr} | {tsr} | {parity} |")
    out.append("\n## Distinct CST node kinds per parser (whole corpus)\n")
    out.append("| textparser kind | count | | tree-sitter kind | count |")
    out.append("|---|---|---|---|---|")
    tp_sorted = sorted(census_tp.items(), key=lambda kv: (-kv[1], kv[0]))
    ts_sorted = sorted(census_ts.items(), key=lambda kv: (-kv[1], kv[0]))
    for i in range(max(len(tp_sorted), len(ts_sorted))):
        tk = f"{tp_sorted[i][0]} | {tp_sorted[i][1]}" if i < len(tp_sorted) else " | "
        sk = f"{ts_sorted[i][0]} | {ts_sorted[i][1]}" if i < len(ts_sorted) else " | "
        out.append(f"| {tk} | | {sk} |")
    out.append(f"\nTotal distinct kinds: textparser={len(census_tp)}, tree-sitter={len(census_ts)}")
    mismatches = [r for r in rows if r[5] == "MISMATCH"]
    out.append(f"\n## Mismatches: {len(mismatches)}")
    for name, st, dc, tpr, tsr, parity in mismatches:
        out.append(f"- **{name}** — tp status `{st}` (reject={tpr}), ts reject={tsr}")
    return "\n".join(out) + "\n", dict(census_tp), dict(census_ts), rows

def constructs_report():
    out = []
    out.append("# Node kinds & tree shape: single-construct comparison\n")
    out.append("`tp` = textparser CFML CST; `ts` = tree-sitter cfml/cfscript "
               "parse tree (anonymous tokens shown quoted).\n")
    for name, spec in CONSTRUCTS.items():
        ext, src = spec if isinstance(spec, tuple) else (".cfm", spec)
        path = os.path.join(WORK, name + ext)
        with open(path, "w") as f:
            f.write(src)
        tp_path = path
        if name in WRAP_FOR_TEXTPARSER:
            tp_path = os.path.join(WORK, name + ".cfm")
            with open(tp_path, "w") as f:
                f.write("<cfscript>\n" + src + "</cfscript>\n")
        tp = tp_parse(tp_path)
        sexp, ts_err = ts_parse(path)
        tp_txt = []
        if tp.get("cst"):
            cst_lines(tp["cst"], tp_txt)
        out.append(f"\n## {name}{ext}\n")
        out.append("```\n" + src + "```\n")
        out.append(f"- textparser status: `{tp.get('status')}`")
        out.append(f"- tree-sitter root error: `{ts_err}`\n")
        out.append("**textparser CST**\n\n```\n" + ("\n".join(tp_txt) if tp_txt else "(no cst)") + "\n```\n")
        out.append("**tree-sitter CST**\n\n```\n" + "\n".join(sexp_outline(sexp)) + "\n```\n")
    return "\n".join(out) + "\n"

def main():
    fix_md, census_tp, census_ts, rows = fixtures_report()
    with open(os.path.join(HERE, "fixtures_report.md"), "w") as f:
        f.write(fix_md)
    cons_md = constructs_report()
    with open(os.path.join(HERE, "constructs_report.md"), "w") as f:
        f.write(cons_md)
    summary = {
        "textparser_distinct_kinds": len(census_tp),
        "treesitter_distinct_kinds": len(census_ts),
        "construct_count": len(CONSTRUCTS),
        "mismatch_count": sum(1 for r in rows if r[5] == "MISMATCH"),
    }
    json.dump(summary, open(os.path.join(HERE, "summary.json"), "w"), indent=1)
    print("wrote fixtures_report.md, constructs_report.md, summary.json")

if __name__ == "__main__":
    main()