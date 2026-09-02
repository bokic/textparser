#!/usr/bin/env python3
"""Differential CST verification of the textparser TypeScript grammar against
tree-sitter-typescript.

Run from the repository root after tests/treesitter_compare/build.sh:
    python3 tests/treesitter_compare/compare.py

Emits into tests/treesitter_compare/:
  fixtures_report.md  - acceptance parity over the committed TS fixture corpus
                        plus a node-kind census (kinds used by each parser)
  constructs_report.md- node-kind + tree-shape comparison for a small corpus of
                        single-construct files (textparser CST vs tree-sitter)
  work/               - per-file raw CSTs used to build the reports
"""
import json, os, subprocess, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))          # repo root
FIX = os.path.join(ROOT, "tests", "docker", "fixtures", "typescript")
CSTD = os.path.join(HERE, "cstdump")
DUMP = os.path.join(HERE, "dump")
WORK = os.path.join(HERE, "work")
os.makedirs(WORK, exist_ok=True)

# ---------------------------------------------------------------- snippets ---
# (stem, extension, source). One self-contained construct each. Keep it valid
# TypeScript; profiles are chosen by extension like the real parser does.
CONSTRUCTS = {
    "var_stmt": (".ts", 'const answer: number = 42;\n'),
    "binary_expr": (".ts", "const r = a + b * (c - d) % e;\n"),
    "ternary": (".ts", "const t = cond ? a : b;\n"),
    "fn_decl": (".ts", "function add(a: number, b?: string, ...r: boolean[]): number { return a + b; }\n"),
    "arrow": (".ts", "const f = (x: number): number => x * 2;\n"),
    "async_arrow": (".ts", "const g = async (a, b) => a + b;\n"),
    "class_decl": (".ts", "class C extends B implements I { private readonly x: number = 1; m(): void {} }\n"),
    "accessor": (".ts", "class C { override readonly accessor auto = 2; }\n"),
    "interface": (".ts", "interface P { x: number; m(a: string): void; }\n"),
    "type_alias": (".ts", "type T = { a: number } | string[];\n"),
    "generic_type": (".ts", "type U<T> = T extends string ? { v: T } : never;\n"),
    "if_stmt": (".ts", "function f(a: number) { if (a > 0) return 1; else { a--; } }\n"),
    "for_stmt": (".ts", "for (let i = 0; i < 10; i++) { use(i); }\n"),
    "object_lit": (".ts", "const o = { a: 1, b, \"c\": 2, [k]: 3, ...spread };\n"),
    "array_lit": (".ts", "const arr = [1, 2, , 3, ...[4]];\n"),
    "destructure": (".ts", "const { x, y: renamed = 5, ...rest } = source;\n"),
    "import_stmt": (".ts", 'import { x as y, type T } from "./m";\n'),
    "export_stmt": (".ts", 'export * from "./m";\n'),
    "import_defer": (".ts", 'import defer * as deferred from "./d";\n'),
    "enum_decl": (".ts", "enum Color { Red, Green = 2, Blue = \"b\" }\n"),
    "namespace": (".ts", "namespace Util { export const helper = 1; }\n"),
    "call_chain": (".ts", "const v = obj.method(1, 2).prop[3];\n"),
    "template": (".ts", "const tpl = tag`hi ${name}!`;\n"),
    "jsx_attr": (".tsx", "const el = <div onClick={() => go()} disabled>Hi {name}</div>;\n"),
    "dts_ambient": (".d.ts", "declare var gv: number;\ndeclare function gf(a: string): void;\n"),
    "new_expr": (".ts", "const made = new Foo.Bar(1);\n"),
}

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

def sexp_kind_walk(toks, pos, counter):
    # tokens: list of ('('|')'|name|'Q...'). Reconstructed by sexp_to_tokens.
    pass

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

def build_ts_tree(toks):
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
                # bare node kind with no further structure (leaf token node)
                kids.append(("tok", [], t))
    return node()

def ts_kinds(toks, counter):
    tree = build_ts_tree(toks)
    def walk(n):
        counter[n[0]] += 1
        for k in n[1]:
            if k[0] == "tok":
                counter["tok:" + k[2]] += 1
            else:
                walk(k)
    walk(tree)

# ------------------------------------------------------- fixture corpus -----

def fixtures_report():
    out = []
    out.append("# Fixture corpus parity: textparser vs tree-sitter-typescript\n")
    out.append("textparser statuses/diagnostics come from the committed golden CSTs "
               "(`tests/docker/fixtures/typescript/golden/*.json`); tree-sitter "
               "reports whether its root tree contains an ERROR node. A fixture is "
               "taken to be *rejected* by textparser when its status is not `OK` or "
               "it carries at least one diagnostic; tree-sitter rejects when it has "
               "an ERROR node.\n")
    header = "| fixture | tp status | tp diagnostics | tp rejects | ts rejects | parity |"
    out.append(header)
    out.append("|---|---|---|---|---|---|")
    census_tp = collections.Counter()
    census_ts = collections.Counter()
    rows = []
    for which in ("valid", "invalid"):
        d = os.path.join(FIX, which)
        for fn in sorted(os.listdir(d)):
            if not fn.endswith((".ts", ".tsx", ".d.ts", ".mts", ".cts")):
                continue
            path = os.path.join(d, fn)
            stem = os.path.splitext(fn)[0]
            golden = os.path.join(FIX, "golden", stem + ".json")
            tp_status = "n/a"
            tp_rejects = "n/a"
            diag_count = 0
            if os.path.exists(golden):
                g = json.load(open(golden))
                tp_status = g.get("status")
                diags = g.get("diagnostics") or []
                diag_count = len(diags)
                tp_rejects = (tp_status != "OK") or (diag_count > 0)
                if g.get("cst"):
                    kind_walk(g["cst"], census_tp)
                else:
                    census_tp["<no-cst>"] += 1
            sexp, ts_err = ts_parse(path)
            ts_rejects = ts_err
            toks = sexp_to_tokens(sexp)
            ts_kinds(toks, census_ts)
            if tp_rejects == "n/a":
                parity = "OK"
            else:
                parity = "OK" if tp_rejects == ts_rejects else "MISMATCH"
            rows.append((fn, which, tp_status, diag_count, tp_rejects, ts_rejects, parity))
    for fn, which, st, dc, tpr, tsr, parity in rows:
        out.append(f"| {fn} ({which}) | {st} | {dc} | {tpr} | {tsr} | {parity} |")
    out.append("\n## Distinct CST node kinds per parser (whole corpus)\n")
    out.append("| textparser kind | count | | tree-sitter kind | count |")
    out.append("|---|---|---|---|---|")
    tp_sorted = sorted(census_tp.items(), key=lambda kv: (-kv[1], kv[0]))
    ts_sorted = sorted(census_ts.items(), key=lambda kv: (-kv[1], kv[0]))
    for (tk, tc), (sk, sc) in zip(tp_sorted, ts_sorted):
        out.append(f"| {tk} | {tc} | | {sk} | {sc} |")
    for (tk, tc) in tp_sorted[len(ts_sorted):]:
        out.append(f"| {tk} | {tc} | | | |")
    for (sk, sc) in ts_sorted[len(tp_sorted):]:
        out.append(f"| | | | {sk} | {sc} |")
    out.append(f"\nTotal distinct kinds: textparser={len(census_tp)}, tree-sitter={len(census_ts)}")
    return "\n".join(out) + "\n", dict(census_tp), dict(census_ts), rows

# ------------------------------------------------------------ constructs ----

def constructs_report():
    out = []
    out.append("# Node kinds & tree shape: single-construct comparison\n")
    out.append("`tp` = textparser grammar CST (from `definitions/typescript_definition.json`); "
               "`ts` = tree-sitter-typescript parse tree (anonymous tokens shown quoted).\n")
    for name, (ext, src) in CONSTRUCTS.items():
        path = os.path.join(WORK, name + ext)
        with open(path, "w") as f:
            f.write(src)
        tp = tp_parse(path)
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
        with open(os.path.join(WORK, name + ".tp.txt"), "w") as f:
            f.write("status=" + str(tp.get("status")) + "\n" + "\n".join(tp_txt) + "\n")
        with open(os.path.join(WORK, name + ".ts.txt"), "w") as f:
            f.write("root_error=" + str(ts_err) + "\n" + "\n".join(sexp_outline(sexp)) + "\n")
    return "\n".join(out) + "\n"

def main():
    os.makedirs(WORK, exist_ok=True)
    fix_md, census_tp, census_ts, rows = fixtures_report()
    json.dump(rows, open(os.path.join(HERE, "fixture_rows.json"), "w"), indent=1)
    with open(os.path.join(HERE, "fixtures_report.md"), "w") as f:
        f.write(fix_md)
    cons_md = constructs_report()
    with open(os.path.join(HERE, "constructs_report.md"), "w") as f:
        f.write(cons_md)
    summary = {
        "textparser_distinct_kinds": len(census_tp),
        "treesitter_distinct_kinds": len(census_ts),
        "construct_count": len(CONSTRUCTS),
    }
    json.dump(summary, open(os.path.join(HERE, "summary.json"), "w"), indent=1)
    print("wrote fixtures_report.md, constructs_report.md, summary.json")

if __name__ == "__main__":
    main()
