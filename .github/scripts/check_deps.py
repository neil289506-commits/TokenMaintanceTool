#!/usr/bin/env python3
"""Checks a staged Windows app folder WITHOUT running it (the ARM64 build is cross-compiled on an x64 runner,
so it cannot be executed in CI):

  1. every .exe / .dll in the folder has the expected CPU architecture (catches an x64 DLL copied into an ARM64 package)
  2. every DLL they import is either shipped in the folder, or a Windows system DLL (api-ms-*, or present in System32)

usage: check_deps.py <folder> [--machine arm64|x64] [--dumpbin dumpbin.exe] [--system-dir C:\\Windows\\System32]
dumpbin comes from the MSVC environment (ilammy/msvc-dev-cmd). Exit status 1 on any problem."""
import argparse, os, re, subprocess, sys

MACHINES = {"arm64": "AA64", "x64": "8664"}


def dumpbin(exe, path):
    p = subprocess.run([exe, "/nologo", "/dependents", "/headers", path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError("dumpbin failed for %s:\n%s" % (path, p.stdout))
    return p.stdout


def parse(out):
    """-> (machine code like 'AA64', [imported dll names])"""
    m = re.search(r"^\s*([0-9A-Fa-f]{4}) machine \(", out, re.M)
    deps, active = [], False
    for line in out.splitlines():
        if "Image has the following dependencies" in line:
            active = True
        elif "Image has the following delay load dependencies" in line or line.strip() == "Summary":
            active = False
        elif active:
            d = re.match(r"^\s+(\S+\.dll)\s*$", line, re.I)
            if d:
                deps.append(d.group(1))
    return (m.group(1).upper() if m else None), deps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("folder")
    ap.add_argument("--machine", default="arm64", choices=sorted(MACHINES))
    ap.add_argument("--dumpbin", default="dumpbin")
    ap.add_argument("--system-dir", default=os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32"))
    a = ap.parse_args()

    shipped = {}
    for root, _, files in os.walk(a.folder):
        for f in files:
            if f.lower().endswith((".exe", ".dll")):
                shipped.setdefault(f.lower(), os.path.join(root, f))
    system = {f.lower() for f in os.listdir(a.system_dir)} if os.path.isdir(a.system_dir) else set()

    problems, wanted = [], MACHINES[a.machine]
    for name, path in sorted(shipped.items()):
        machine, deps = parse(dumpbin(a.dumpbin, path))
        rel = os.path.relpath(path, a.folder)
        if machine != wanted:
            problems.append("%s: machine %s, expected %s (%s)" % (rel, machine, wanted, a.machine))
        for d in deps:
            low = d.lower()
            if low in shipped or low in system or low.startswith(("api-ms-", "ext-ms-")):
                continue
            problems.append("%s imports %s, which is neither shipped nor a Windows system DLL" % (rel, d))
    print("checked %d binaries in %s" % (len(shipped), a.folder))
    for p in problems:
        print("::error::" + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
