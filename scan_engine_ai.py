import os
import re
from collections import defaultdict

BASE = "/storage/emulated/0/Buku saya/FauzanEngine/Source/NeoEngine"

report = []
includes_map = defaultdict(set)
files_set = set()

def read_file(path):
    try:
        with open(path, 'r', errors='ignore') as f:
            return f.read()
    except:
        return ""

def analyze_file(path):
    content = read_file(path)
    lines = content.splitlines()

    result = {
        "file": path,
        "lines": len(lines),
        "functions": 0,
        "issues": []
    }

    # fungsi detection
    funcs = re.findall(r'\w+\s+\w+\(.*\)\s*\{', content)
    result["functions"] = len(funcs)

    # bracket check
    if content.count("{") != content.count("}"):
        result["issues"].append("Bracket mismatch")

    # chrono error detection
    if "std::chrono" in content and ">" in content and ";" not in content:
        result["issues"].append("Broken chrono syntax")

    # no function
    if result["functions"] == 0:
        result["issues"].append("No functions")

    # include parsing
    includes = re.findall(r'#include\s+"(.+)"', content)
    for inc in includes:
        includes_map[path].add(inc)

    # empty logic
    if len(lines) < 15:
        result["issues"].append("Too short")

    return result

def scan():
    for root, dirs, files in os.walk(BASE):
        for f in files:
            if f.endswith(".cpp") or f.endswith(".h"):
                full = os.path.join(root, f)
                files_set.add(full)

                res = analyze_file(full)

                if res["issues"]:
                    report.append(res)

def detect_orphans():
    referenced = set()

    for f, incs in includes_map.items():
        for inc in incs:
            referenced.add(inc)

    orphans = []
    for f in files_set:
        name = os.path.basename(f)
        if name not in referenced:
            orphans.append(f)

    return orphans

scan()
orphans = detect_orphans()

output_path = "/storage/emulated/0/Buku saya/engine_audit_report.txt"

with open(output_path, "w") as out:
    out.write("=== ENGINE AUDIT REPORT ===\n\n")

    for r in report:
        out.write(f"[FILE] {r['file']}\n")
        out.write(f"Lines: {r['lines']}\n")
        out.write(f"Functions: {r['functions']}\n")
        out.write("Issues:\n")
        for i in r["issues"]:
            out.write(f" - {i}\n")
        out.write("\n")

    out.write("\n=== ORPHAN FILES ===\n")
    for o in orphans:
        out.write(o + "\n")

print("REPORT SAVED:", output_path)
