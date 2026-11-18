"""Run session tests in disposable private folders without touching a user's Praat session."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time
import wave
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("--test-exe", type=Path, required=True)
parser.add_argument("--praat-exe", type=Path, required=True)
parser.add_argument("--periodic", action="store_true", help="Also wait for a real five-minute autosave and test forced-termination recovery")
parser.add_argument("--large-package", action="store_true", help="Also generate and verify a ZIP64 archive using about 8 GiB of temporary space")
args = parser.parse_args()
base = Path(tempfile.mkdtemp(prefix="praat-session-tests-"))
print(f"Test data: {base}", flush=True)

def command(mode, folder, extra=()):
    folder.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, PRAAT_SESSION_TEST_MODE=mode)
    cmd = [str(args.test_exe.resolve()), f"--pref-dir={folder}", "--no-plugins", *extra]
    return cmd, env

def run(mode, folder, extra=(), timeout=45):
    cmd, env = command(mode, folder, extra)
    result = subprocess.run(cmd, env=env, capture_output=True, timeout=timeout)
    print(result.stdout.decode("utf-8", errors="replace").strip(), flush=True)
    if result.returncode:
        raise RuntimeError(f"{mode} failed ({result.returncode}): {result.stderr.decode('utf-8', errors='replace')}")

run("core", base / "Unicode å")
history = base / "recovery-history"
run("seed-recovery-history", history)
run("recovery-history", history)
run("named-project", base / "named-project")
package = base / "package-source"
run("package-project", package)
received = base / "received" / "moved-project"
with zipfile.ZipFile(package / "Portable å.zip") as archive:
    assert archive.namelist() == ["Portable å.PraatProject", "Recordings/1/source å.wav"]
    assert archive.testzip() is None
    assert archive.read("Recordings/1/source å.wav") == (package / "source å.wav").read_bytes()
    archive.extractall(received)
(package / "source å.wav").rename(package / "unavailable.wav")
run("open-package", received)
relocated = base / "relocated-again"
received.rename(relocated)
run("open-package", relocated)
recording = relocated / "Recordings" / "1" / "source å.wav"
with wave.open(str(recording), "wb") as wrong:
    wrong.setparams((1, 2, 16000, 1, "NONE", "not compressed"))
    wrong.writeframes(b"\0\0")
run("reject-package-recording", relocated)
recording.rename(relocated / "wrong.wav")
(relocated / "relocated.wav").write_bytes((package / "unavailable.wav").read_bytes())
run("cancel-relink", relocated)
run("accept-relink", relocated)
print("ZIP and relink checks passed, including relocation, duplicate sources, mismatched audio rejection and cancellation", flush=True)

if args.large_package:
    large = base / "zip64"
    large.mkdir()
    big_media, tail_media = large / "large.wav", large / "small.wav"
    archive_path = large / "large.zip"
    try:
        with wave.open(str(tail_media), "wb") as wav:
            wav.setparams((1, 2, 16000, 1, "NONE", "not compressed"))
            wav.writeframes(b"\0\0")
        big_media.write_bytes(tail_media.read_bytes())
        with big_media.open("r+b") as media:
            media.truncate((1 << 32) + 700)
        run("package-large", large, timeout=180)
        with zipfile.ZipFile(archive_path) as archive:
            oversized = archive.getinfo("Recordings/1/large.wav")
            tail = archive.getinfo("Recordings/2/small.wav")
            assert oversized.file_size == big_media.stat().st_size
            assert tail.header_offset > (1 << 32)
            assert archive.read(tail) == tail_media.read_bytes()
        print("ZIP64 entry size, central directory, and offsets beyond 4 GiB verified", flush=True)
    finally:
        for path in (archive_path, big_media, tail_media):
            path.unlink(missing_ok=True)

restart = base / "restart"
run("seed-exit", restart)
assert len(list((restart / "SessionBackups").glob("session-*.Collection"))) == 1
(restart / "source å.wav").rename(restart / "moved.wav")
run("repair-exit", restart)
assert len(list((restart / "SessionBackups").glob("session-*.Collection"))) == 2
run("no-pref-exit", restart, ["--no-pref-files"])
assert len(list((restart / "SessionBackups").glob("session-*.Collection"))) == 2
run("restore-exit", restart)
run("empty-exit", restart)
assert len(list((restart / "SessionBackups").glob("session-*.Collection"))) == 2

concurrent = base / "concurrent"
children = []
for _ in range(2):
    cmd, env = command("concurrent-exit", concurrent)
    children.append(subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE))
for child in children:
    try:
        stdout, stderr = child.communicate(timeout=45)
    except subprocess.TimeoutExpired:
        child.kill()
        child.communicate()
        raise
    assert child.returncode == 0, stderr.decode("utf-8", errors="replace")
assert len(list((concurrent / "SessionBackups").glob("session-*.Collection"))) == 2
print("Concurrent application exits created two distinct complete backups", flush=True)

batch = base / "batch"
batch.mkdir()
script = batch / "smoke.praat"
script.write_text('Create Sound from formula: "batch", 1, 0, 0.01, 16000, "0.1"\nwriteInfoLine: "Batch mode passed"\n', encoding="utf-8")
result = subprocess.run([str(args.praat_exe.resolve()), f"--pref-dir={batch}", "--no-plugins", "--run", str(script)],
                        capture_output=True, timeout=45)
assert result.returncode == 0, result.stderr
assert not (batch / "SessionBackups").exists()
print("Release executable batch mode created no session backup", flush=True)

if args.periodic:
    periodic = base / "periodic"
    cmd, env = command("periodic-crash", periodic)
    retry = base / "periodic-retry"
    retry_cmd, retry_env = command("periodic-retry", retry)
    with (periodic / "process.log").open("wb") as log, (retry / "process.log").open("wb") as retry_log:
        started = time.monotonic()
        child = subprocess.Popen(cmd, env=env, stdout=log, stderr=subprocess.STDOUT)
        retry_child = subprocess.Popen(retry_cmd, env=retry_env, stdout=retry_log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 340
            next_report = time.monotonic() + 45
            while not list((periodic / "SessionBackups").glob("session-*.Collection")):
                if child.poll() is not None:
                    raise RuntimeError(f"Periodic autosave test exited early: {child.returncode}")
                if retry_child.poll() not in (None, 0):
                    raise RuntimeError(f"Periodic retry test failed: {retry_child.returncode}")
                if time.monotonic() >= deadline:
                    raise TimeoutError("No default five-minute autosave appeared")
                if time.monotonic() >= next_report:
                    print("Waiting for default five-minute timer (test process only)", flush=True)
                    next_report += 45
                time.sleep(0.1)
            elapsed = time.monotonic() - started
            assert 299 <= elapsed < 340, f"Default autosave interval was {elapsed:.1f}s"
            print(f"Application's own timer woke an idle session after {elapsed:.1f}s", flush=True)
            assert retry_child.wait(timeout=15) == 0
        finally:
            if child.poll() is None:
                child.kill()
            child.wait(timeout=10)
            if retry_child.poll() is None:
                retry_child.kill()
            retry_child.wait(timeout=10)
    print((periodic / "process.log").read_text(encoding="utf-8", errors="replace").strip(), flush=True)
    print((retry / "process.log").read_text(encoding="utf-8", errors="replace").strip(), flush=True)
    assert len(list((retry / "SessionBackups").glob("session-*.Collection"))) == 3
    assert len(list((periodic / "SessionBackups").glob("session-*.Collection"))) == 1
    run("crash-restore-exit", periodic)
    assert len(list((periodic / "SessionBackups").glob("session-*.Collection"))) == 1
    print("Forced termination recovered the periodic snapshot, exact data and unsaved drafts", flush=True)
print("All session autosave checks passed", flush=True)
