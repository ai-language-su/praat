"""Ensure public Sound actions and inherited time commands receive reviewed help.

Run from any directory: python test/verify_sound_tooltips.py
This is a development coverage check, not a runtime command catalogue.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HELP = ROOT / "sys/praat_SoundCommandHelp.inc"
RECORDS = re.findall(r'SOUND_HELP\(U"([^"]+)"', HELP.read_text(encoding="utf-8"))
assert len(RECORDS) == len(set(RECORDS)), "Duplicate help key"


def registrations():
    for folder in ("fon", "dwtools", "LPC", "EEG", "artsynth"):
        for path in sorted((ROOT / folder).glob("*.cpp")):
            if path.name == "praat_TimeFunction.cpp":
                owner = "klas"  # both initializers are called for Sound
            else:
                owner = "classSound"
            text = path.read_text(encoding="utf-8")
            pattern = (r'praat_addAction1\s*\(\s*' + owner +
                       r'\s*,\s*\d+\s*(?:/\*.*?\*/\s*)?,\s*U"([^"]*)"\s*,\s*([^;]+?)\);')
            matches = list(re.finditer(pattern, text, re.S))
            starts = {match.start() for match in matches}
            for beginning in re.finditer(r'praat_addAction1\s*\(\s*' + owner + r'\s*,', text):
                assert beginning.start() in starts, f"Unparsed registration in {path}: {beginning.group()}"
            for match in matches:
                name = match[1].split(" || ")[0]
                # Save commands live in the top Save menu, not the right panel.
                if name.startswith(("--", "Save ", "Append to existing")):
                    continue
                if any(flag in match[2] for flag in ("HIDDEN", "DEPRECATED")):
                    continue
                yield name, path.relative_to(ROOT), text[:match.start()].count("\n") + 1


if __name__ == "__main__":
    required = list(registrations())
    missing = [(name, str(path), line) for name, path, line in required if name not in RECORDS]
    assert not missing, f"Public Sound actions need review: {missing}"
    print(f"Coverage passed: {len(required)} public Sound/time entries, {len(RECORDS)} reviewed keys")
