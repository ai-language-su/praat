"""Build the Windows hover integration test using this build's actual linker flags.

Example (after building Praat-editor-help.exe and capturing its build log):
  python test/build_hover_native_test.py --build-log C:/msys64/tmp/praat-editor-help-build.log \
      --compiler C:/msys64/mingw64/bin/g++.exe --output C:/msys64/tmp/praat-hover-native-test.exe
The test operates only on its own generated Sound/Spectrum editor instances.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--build-log", type=Path, required=True)
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
compiler = str(options.compiler.resolve())
env = dict(os.environ)
env["PATH"] = str(options.compiler.resolve().parent) + os.pathsep + env["PATH"]
obj = options.output.with_suffix(".o").resolve()
includes = ["-I" + p for p in ("kar", "melder", "sys", "dwsys", "stat", "fon", "foned", "dwtools")]
subprocess.run([compiler, "-std=gnu++17", "-municode", "-D_FILE_OFFSET_BITS=64", "-D_WIN32_WINNT=0x0602",
                "-march=x86-64-v3", "-O1", *includes, "-c", "test/GuiHoverHelp_native_test.cpp", "-o", str(obj)],
               cwd=root, env=env, check=True)
lines = options.build_log.read_text(encoding="utf-8").splitlines()
start = next(i for i, line in enumerate(lines) if " -o Praat-editor-help.exe main/main_Praat.o" in line)
command = lines[start].rstrip("\\").strip()
for line in lines[start + 1:]:
    command += " " + line.rstrip("\\").strip()
    if not line.endswith("\\"):
        break
args = shlex.split(command)
args[0] = compiler
args[args.index("Praat-editor-help.exe")] = str(options.output.resolve())
args[args.index("main/main_Praat.o")] = str(obj)
subprocess.run(args, cwd=root, env=env, check=True)
print("Native hover integration test linked")
