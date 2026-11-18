"""Link the Windows theme integration test against the current application libraries."""
import argparse, os, shlex, subprocess
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--build-log', type=Path, required=True)
p.add_argument('--compiler', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
env = dict(os.environ)
env['PATH'] = str(a.compiler.resolve().parent) + os.pathsep + env['PATH']
obj = a.output.resolve().with_suffix('.o')
subprocess.run([str(a.compiler), '-std=gnu++17', '-municode', '-D_FILE_OFFSET_BITS=64', '-D_WIN32_WINNT=0x0602', '-march=x86-64-v3', '-O1',
                *['-I'+d for d in ('sys','kar','melder','dwsys','stat','fon','foned','dwtools')], '-c', 'test/GuiTheme_test.cpp', '-o', str(obj)], cwd=root, env=env, check=True)
lines = a.build_log.read_text(encoding='utf-8').splitlines()
start = next(i for i,line in enumerate(lines) if ' -o ' in line and ' main/main_Praat.o' in line)
command = lines[start].rstrip('\\').strip()
for line in lines[start+1:]:
    command += ' ' + line.rstrip('\\').strip()
    if not line.endswith('\\'): break
args = shlex.split(command)
args[0] = str(a.compiler)
args[args.index('-o') + 1] = str(a.output.resolve())
args[args.index('main/main_Praat.o')] = str(obj)
subprocess.run(args,cwd=root,env=env,check=True)
print('Theme integration test linked')
