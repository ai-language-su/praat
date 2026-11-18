"""Build the Windows session integration test with the application's real link flags."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--build-log", type=Path, required=True)
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--source", default="test/SessionAutosave_test.cpp")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
compiler = str(args.compiler.resolve())
env = dict(os.environ)
env["PATH"] = str(args.compiler.resolve().parent) + os.pathsep + env["PATH"]
obj = args.output.resolve().with_suffix(".o")
subprocess.run([compiler, "-std=gnu++17", "-municode", "-D_FILE_OFFSET_BITS=64", "-D_WIN32_WINNT=0x0602",
                "-march=x86-64-v3", "-O1", *["-I" + p for p in ("kar", "melder", "sys", "dwsys", "stat", "fon", "foned", "dwtools")],
                "-c", args.source, "-o", str(obj)], cwd=root, env=env, check=True)
lines = args.build_log.read_text(encoding="utf-8").splitlines()
start = next(i for i, line in enumerate(lines) if " -o " in line and " main/main_Praat.o" in line)
command = lines[start].rstrip("\\").strip()
for line in lines[start + 1:]:
    command += " " + line.rstrip("\\").strip()
    if not line.endswith("\\"):
        break
link = shlex.split(command)
link[0] = compiler
link[link.index("-o") + 1] = str(args.output.resolve())
link[link.index("main/main_Praat.o")] = str(obj)
subprocess.run(link, cwd=root, env=env, check=True)
print(f"Linked {args.source}")
