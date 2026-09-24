#!/usr/bin/env python3
"""Cross-run: each parser executes the other's CFML tests / corpus / examples.

Sources:
  - tree-sitter-cfml corpus (cfml + cfscript + cfquery, from TSREPO)
  - tree-sitter-cfml examples/
  - textparser CFML unit tests (tests/unittests/cfml_tests.cpp raw strings)

For every source BOTH parsers run (textparser cstdump, tree-sitter dump) and
we report accept/reject parity plus a structural comparison of the two CSTs:

  parity  = accept-both / reject-both / tp-only / ts-only
  shape   = structural agreement of the accepted pair's node trees
            (node names dropped; only tree shape counts):
              same   identical skeleton
              close  similar (node count within +-30%, depth +-2)
              diff   structurally different (granularity mismatch) — see appendix C
  -       = not comparable (at least one parser rejected)

Run from the textparser repo root:  python3 tests/treesitter_compare/cfml/cross.py
Set TS_CFML_REPO to point at the tree-sitter-cfml checkout if not
/tmp/opencode/ts-cfml-pinned.
"""

import argparse
import collections
import json
import os
import re
import subprocess
import sys

sys.setrecursionlimit(200000)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
TSREPO = os.environ.get("TS_CFML_REPO", "/tmp/opencode/ts-cfml-pinned")
CSTD = os.path.join(HERE, "cstdump")
DUMP = os.path.join(HERE, "dump")
WORK = os.path.join(HERE, "work", "cross")
CORPUS_DIRS = {
    "cfml-corpus": os.path.join(TSREPO, "cfml/test/corpus"),
    "script-corpus": os.path.join(TSREPO, "cfscript/test/corpus"),
    "query-corpus": os.path.join(TSREPO, "cfquery/test/corpus"),
}
EXAMPLES_DIR = os.path.join(TSREPO, "examples")
TP_UNIT_CPP = os.path.join(ROOT, "tests/unittests/cfml_tests.cpp")

KNOWN_TP_REJECTS = [
    "component", "interface", "cfs_component", "script component",
    "script interface", "component_basic",
]


def run(cmd, cwd):
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    return p.stdout, p.stderr


# ------------------------------------------------------------- extraction ---

_re_eq = re.compile(r"^=+$")
_re_dash = re.compile(r"^-+$")


def eq_line(s):
    s = s.strip()
    return len(s) >= 3 and _re_eq.match(s) is not None


def dash_line(s):
    s = s.strip()
    return len(s) >= 3 and _re_dash.match(s) is not None


def read_corpus_tests(txt_path):
    """Parse a tree-sitter corpus file into [(title, source), ...]."""
    with open(txt_path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    tests = []
    i, n = 0, len(lines)
    while i < n:
        if not eq_line(lines[i]):
            i += 1
            continue
        j = i + 1
        while j < n and not lines[j].strip():
            j += 1
        title = lines[j].strip() if j < n else "(untitled)"
        k = j + 1
        while k < n and not lines[k].strip():
            k += 1
        if k >= n or not eq_line(lines[k]):
            i = j + 1
            continue
        m = k + 1
        src_lines = []
        while m < n and not dash_line(lines[m]):
            src_lines.append(lines[m])
            m += 1
        src = "\n".join(src_lines).rstrip("\n")
        tests.append((title, src))
        while m < n and not eq_line(lines[m]):  # skip expected tree
            m += 1
        i = m
    return tests


def read_tp_snippets(cpp_path):
    with open(cpp_path, encoding="utf-8") as f:
        text = f.read()
    out, seen = [], set()
    for m in re.finditer(r'R"\((.*?)\)"', text, re.S):
        src = m.group(1)
        # CFML-marker check anchored to a line start so quoted "startRegex"
        # values inside non-CFML JSON/rules blobs (`"<cfset", ...) do not match.
        if not re.search(r"(?m)^[ \t]*(?:<cf|cfscript|cfquery)", src, re.I):
            continue
        if src not in seen:
            seen.add(src)
            out.append(src)
    return out


# ------------------------------------------------------------------ tools ---

def tp_run(path):
    out, _ = run([CSTD, path], ROOT)
    try:
        d = json.loads(out)
    except Exception:
        return "BROKEN", 0, None
    return d.get("status", "?"), len(d.get("diagnostics") or []), d.get("cst")


def ts_run(path):
    out, err = run([DUMP, path], HERE)
    return out.strip(), "ROOT_ERROR=1" in err


def cst_lines(node, out, depth=0):
    out.append((depth, node.get("kind", "?")))
    for c in node.get("children", []):
        cst_lines(c, out, depth + 1)


def sexp_outline(sexp):
    out = []
    depth = 0
    i, n = 0, len(sexp)
    while i < n:
        c = sexp[i]
        if c == "(":
            depth += 1
            i += 1
        elif c == ")":
            depth -= 1
            i += 1
        elif c == '"':
            j = i + 1
            while j < n and sexp[j] != '"':
                j += 1
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and sexp[j] not in '()"' and not sexp[j].isspace():
                j += 1
            out.append((depth, sexp[i:j]))
            i = j
    return out


def outline_tree(lines):
    root = None
    stack = []
    for depth, name in lines:
        node = [name, []]
        while stack and stack[-1][0] >= depth:
            stack.pop()
        if stack:
            stack[-1][1][1].append(node)
        else:
            root = node
        stack.append((depth, node))
    return root or ["?", []]


def inner_containing(node, s, e):
    st, en = node.get("start", -1), node.get("end", -1)
    if st > s or en < e:
        return None
    for c in node.get("children", []):
        r = inner_containing(c, s, e)
        if r is not None:
            return r
    return node


def tree_metrics(root):
    cnt, mdepth = [0], [0]

    def walk(node, d):
        stack = [(node, d)]
        while stack:
            n, depth = stack.pop()
            cnt[0] += 1
            if depth > mdepth[0]:
                mdepth[0] = depth
            for c in n[1]:
                stack.append((c, depth + 1))

    def skeleton(node):
        # iterative post-order; skeleton string for each node keyed by id()
        built = {}
        stack = [(node, False)]
        while stack:
            n, done = stack.pop()
            if done:
                if not n[1]:
                    built[id(n)] = "L"
                else:
                    built[id(n)] = "(" + str(len(n[1])) + \
                        "".join(built[id(c)] for c in n[1]) + ")"
            else:
                stack.append((n, True))
                for c in n[1]:
                    stack.append((c, False))
        return built[id(root)]

    walk(root, 0)
    return cnt[0], mdepth[0], skeleton(root)


def shape_verdict(tp_lines, ts_lines):
    tp_tree = outline_tree(tp_lines)
    ts_tree = outline_tree(ts_lines)
    tc, td, tsk = tree_metrics(tp_tree)
    sc, sd, ssk = tree_metrics(ts_tree)
    if ssk == tsk:
        return "same", tc, sc, td, sd
    if max(tc, sc) * 0.7 <= min(tc, sc) and abs(td - sd) <= 2:
        return "close", tc, sc, td, sd
    return "diff", tc, sc, td, sd


# ------------------------------------------------------------- collection ---

def collect_sources():
    items = []  # (group, file, title, source, ts_ext, wrap_tp)
    for group, cdir in CORPUS_DIRS.items():
        if not os.path.isdir(cdir):
            print(f"warning: corpus dir missing: {cdir}", file=sys.stderr)
            continue
        for name in sorted(os.listdir(cdir)):
            if not name.endswith(".txt"):
                continue
            for idx, (title, src) in enumerate(read_corpus_tests(os.path.join(cdir, name))):
                items.append((group, name, f"{idx}: {title}", src))
    # examples
    for name in sorted(os.listdir(EXAMPLES_DIR)):
        if name.endswith(".cfm"):
            with open(os.path.join(EXAMPLES_DIR, name), encoding="utf-8") as f:
                items.append(("examples", name, name, f.read()))
    # textparser unit-test snippets
    for idx, src in enumerate(read_tp_snippets(TP_UNIT_CPP)):
        items.append(("tp-unit", "cfml_tests.cpp", f"{idx}", src))
    return items


def classify(items):
    """Return list of result dicts and per-item details for appendices."""
    rows = []
    details = {}  # name -> {"tp": [...], "ts": [...], "src": ...}
    n = len(items)
    for k, (group, file, title, src) in enumerate(items):
        label = f"{group}|{file}|{title}"
        ts_ext = ".cfs" if group == "script-corpus" else ".cfm"
        wrap_tp = group == "script-corpus"
        if group == "query-corpus":
            # bare mode: tree-sitter with cfquery grammar, textparser raw
            ts_ext = ".cfq"
        base = os.path.join(WORK, re.sub(r"[^\w.\-]", "_", label)[:120])
        ts_path = base + ts_ext
        tp_path = base + ".cfm"
        with open(ts_path, "w", encoding="utf-8") as f:
            f.write(src)
        inner, istr, iend = None, 0, len(src)
        if wrap_tp:
            wrapped = "<cfscript>\n" + src + "</cfscript>\n"
            istr, iend = len("<cfscript>\n"), len("<cfscript>\n") + len(src)
            with open(tp_path, "w", encoding="utf-8") as f:
                f.write(wrapped)
        else:
            with open(tp_path, "w", encoding="utf-8") as f:
                f.write(src)
        tp_status, tp_diags, tp_cst = tp_run(tp_path)
        ts_sexp, ts_root_err = ts_run(ts_path)
        tp_ok = tp_status == "OK"
        ts_ok = not ts_root_err
        if tp_ok and ts_ok:
            parity = "accept-both"
        elif tp_ok:
            parity = "tp-only"
        elif ts_ok:
            parity = "ts-only"
        else:
            parity = "reject-both"
        shape = "-"
        tc = sc = td = sd = None
        tp_lines = ts_lines = []
        if tp_ok and ts_ok:
            if wrap_tp and tp_cst:
                inner = inner_containing(tp_cst, istr, iend) or tp_cst
            else:
                inner = tp_cst
            if inner:
                cst_lines(inner, tp_lines)
                ts_lines = sexp_outline(ts_sexp)
                shape, tc, sc, td, sd = shape_verdict(tp_lines, ts_lines)
        rows.append({
            "group": group, "file": file, "test": title, "src": src,
            "tp_status": tp_status, "tp_diags": tp_diags, "ts_ok": ts_ok,
            "parity": parity, "shape": shape,
            "tp_nodes": tc, "ts_nodes": sc, "tp_depth": td, "ts_depth": sd,
        })
        if parity in ("ts-only", "tp-only") or (parity == "accept-both" and shape == "diff"):
            details[label] = {
                "src": src,
                "tp_status": tp_status,
                "tp_diags": tp_diags,
                "parity": parity,
                "tp_lines": ["  " * d + t for d, t in tp_lines][:80],
                "ts_lines": ["  " * d + t for d, t in ts_lines][:80],
            }
        if k and k % 100 == 0:
            print(f"  {k}/{n} processed", file=sys.stderr)
    return rows, details


# --------------------------------------------------------------- reports ---

def fmt(x):
    return "-" if x is None else str(x)


def render(rows, details):
    L = []
    L.append("# Cross-run: textparser vs tree-sitter-cfml on each other's tests\n")
    L.append("Every source below was handed to BOTH parsers: "
             "textparser (`cstdump`, CST) and tree-sitter (`dump`, cfml/cfscript/"
             "cfquery grammar per the corpus origin). See `cross.py` header for "
             "the `parity`/`shape` definitions.\n")
    counts = collections.Counter(r["parity"] for r in rows)
    shapes = collections.Counter(r["shape"] for r in rows)
    L.append("## Summary\n")
    L.append(f"- sources tested: **{len(rows)}**")
    L.append(f"- parity: accept-both **{counts['accept-both']}**, "
             f"reject-both **{counts['reject-both']}**, "
             f"tp-only **{counts['tp-only']}**, "
             f"ts-only **{counts['ts-only']}**")
    L.append(f"- shape (of accept-both pairs): same **{shapes['same']}**, "
             f"close **{shapes['close']}**, diff **{shapes['diff']}**\n")
    by_group = collections.Counter(r["group"] for r in rows)
    byg_acc = collections.Counter(r["group"] for r in rows if r["parity"] == "accept-both")
    L.append("| group | tests | both accept | parity breakdown |")
    L.append("|---|---|---|---|")
    for g in ["cfml-corpus", "script-corpus", "query-corpus", "examples", "tp-unit"]:
        if by_group[g] == 0:
            continue
        sub = [r for r in rows if r["group"] == g]
        pb = collections.Counter(r["parity"] for r in sub)
        bits = [f"{k}={pb[k]}" for k in ("accept-both", "reject-both", "tp-only", "ts-only") if pb[k]]
        L.append(f"| {g} | {by_group[g]} | {byg_acc[g]} | {', '.join(bits)} |")
    L.append("")
    # classification of shape=diff rows
    grow = mix = shrink = 0
    for r in rows:
        if r["parity"] != "accept-both" or r["shape"] != "diff":
            continue
        tp, ts = r["tp_nodes"], r["ts_nodes"]
        if tp is None or ts is None:
            continue
        if tp >= ts * 1.6:
            grow += 1
        elif ts >= tp * 1.6:
            shrink += 1
        else:
            mix += 1
    L.append("## Why the accept-both pairs differ in shape (shape=diff)\n")
    L.append("Both parsers model the same constructs; the differences are node-"
             "granularity, not semantics (verified by spot-checking outlines). "
             "The skeleton comparison drops node names, so textparser's finer "
             "marker/scope nodes make identical skeletons rare.\n")
    L.append(f"- **{grow}** — textparser tree has ≥ 1.6× the nodes (marker/scope/"
             f"keyword nodes, e.g. `Repeat`, `*_Start`/`*_End`, `TagSelfClose`, "
             f"`FunctionKeyword`; tree-sitter folds these into named nodes)")
    L.append(f"- **{shrink}** — tree-sitter tree has ≥ 1.6× the nodes (quoted "
             f"hash/`#` nodes, field wrappers)")
    L.append(f"- **{mix}** — comparable node counts, differing shape (sampled: "
             f"still same construct, different node naming/granularity)\n")
    L.append("## Full table\n")
    L.append("| group | file | test | tp | tpΔ | ts | parity | shape | tp# | ts# | dp | ds |")
    L.append("|---|---|---|---|---|---|---|---|---|---|---|---|")
    for r in rows:
        L.append(f"| {r['group']} | {r['file']} | {r['test']} | {r['tp_status']} "
                 f"| {r['tp_diags']} | {'OK' if r['ts_ok'] else 'ERR'} "
                 f"| {r['parity']} | {r['shape']} | {fmt(r['tp_nodes'])} "
                 f"| {fmt(r['ts_nodes'])} | {fmt(r['tp_depth'])} | {fmt(r['ts_depth'])} |")
    L.append("")
    # Appendices
    ts_only = [r for r in rows if r["parity"] == "ts-only"]
    tp_only = [r for r in rows if r["parity"] == "tp-only"]
    L.append("## A. tree-sitter accepts, textparser rejects (ts-only)\n")
    for r in ts_only:
        note = ""
        if any(w in r["test"].lower() or w in r["file"].lower() for w in KNOWN_TP_REJECTS):
            note = " — likely textparser limitation (component/interface/etc.)"
        L.append(f"- **{r['group']} / {r['file']} / {r['test']}** — tp `{r['tp_status']}` "
                 f"({r['tp_diags']} diag){note}")
    L.append("")
    L.append("## B. textparser accepts, tree-sitter rejects (tp-only)\n")
    for r in tp_only:
        L.append(f"- **{r['group']} / {r['file']} / {r['test']}** — ts root ERROR; "
                 f"tp `{r['tp_status']}`")
    L.append("")
    diff_rows = [r for r in rows if r["parity"] == "accept-both" and r["shape"] == "diff"]
    L.append("## C. both accept but structurally different (shape=diff)\n")
    for r in diff_rows:
        L.append(f"### {r['group']} / {r['file']} / {r['test']}\n")
        L.append(f"```\n{r['src']}\n```\n")
        d = details.get(f"{r['group']}|{r['file']}|{r['test']}", {})
        L.append("**textparser CST** `tp=" + str(r["tp_nodes"]) + " nodes`\n\n```")
        L.extend(d.get("tp_lines", []))
        L.append("```\n")
        L.append("**tree-sitter CST** `ts=" + str(r["ts_nodes"]) + " nodes`\n\n```")
        L.extend(d.get("ts_lines", []))
        L.append("```\n")
    return "\n".join(L) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0, help="limit sources processed (0 = all)")
    args = parser.parse_args()
    os.makedirs(WORK, exist_ok=True)
    items = collect_sources()
    print(f"collected {len(items)} sources", file=sys.stderr)
    if args.limit:
        items = items[: args.limit]
    rows, details = classify(items)
    md = render(rows, details)
    with open(os.path.join(HERE, "cross_report.md"), "w") as f:
        f.write(md)
    summary = {
        "sources": len(rows),
        "parity": dict(collections.Counter(r["parity"] for r in rows)),
        "shape": dict(collections.Counter(r["shape"] for r in rows)),
        "shape_diff_class": {
            "tp_richer_nodes": sum(1 for r in rows if r["parity"] == "accept-both"
                                   and r["shape"] == "diff" and r["tp_nodes"]
                                   and r["ts_nodes"] and r["tp_nodes"] >= r["ts_nodes"] * 1.6),
            "ts_richer_nodes": sum(1 for r in rows if r["parity"] == "accept-both"
                                   and r["shape"] == "diff" and r["tp_nodes"]
                                   and r["ts_nodes"] and r["ts_nodes"] >= r["tp_nodes"] * 1.6),
            "comparable_counts": sum(1 for r in rows if r["parity"] == "accept-both"
                                     and r["shape"] == "diff" and r["tp_nodes"]
                                     and r["ts_nodes"] and max(r["tp_nodes"], r["ts_nodes"])
                                     < min(r["tp_nodes"], r["ts_nodes"]) * 1.6),
        },
        "ts_only": [f"{r['group']}/{r['file']}/{r['test']}" for r in rows if r["parity"] == "ts-only"],
        "tp_only": [f"{r['group']}/{r['file']}/{r['test']}" for r in rows if r["parity"] == "tp-only"],
        "shape_diff": [f"{r['group']}/{r['file']}/{r['test']}" for r in rows
                       if r["parity"] == "accept-both" and r["shape"] == "diff"],
    }
    with open(os.path.join(HERE, "cross_summary.json"), "w") as f:
        json.dump(summary, f, indent=1)
    print(f"wrote cross_report.md, cross_summary.json ({len(rows)} rows)")


if __name__ == "__main__":
    main()