# Praat · Unofficial Windows fork

**OpenAI Codex developed the changes in this fork.**

### Added since official Praat 7.0.03

- **Autosave and recovery** with five-minute backups by default, snapshot previews, and reversible removal.
- **Named projects** that restore open editors, zoom, and selections. Relink missing recordings or package a project with its audio.
- **Batch Analysis** with reusable presets, previews, and CSV export for duration, F1/F2, and F0.
- **Corpus annotation tools** to find labels across TextGrids, jump to a match, and reuse tiers and allowed labels.
- **Search Praat** finds commands by name or task.
- **Contextual help** explains commands where you use them.
- **Windows appearance settings** add themes and optional button icons.
- **Faster opening on Windows** lets you drop files into Objects and double-click objects to open them in View & Edit.

### Install on Windows

Praat is distributed as a portable app. It requires Windows 10 or 11 and an x64 processor with x86-64-v3 support.

1. Download [Praat-Windows-x64v3.zip](https://github.com/ai-language-su/praat/releases/latest/download/Praat-Windows-x64v3.zip).
2. Right-click the ZIP in File Explorer, choose **Extract All**, and select where you want to keep Praat.
3. Open the extracted folder and double-click **Praat.exe**.

### Build from source

Follow the [MSYS2 setup instructions](HOW_TO_BUILD_ONE.md#1-developing-praat-for-windows), then run the following from the source folder in the **MSYS2 MINGW64** shell. The build creates `Praat.exe` in that folder.

```sh
make PRAAT_ARCH=x64v3 PRAAT_COMPILER=gcc -j4
```

[Releases and matching source](https://github.com/ai-language-su/praat/releases/latest) · [Official Praat](https://github.com/praat/praat.github.io)

Unofficial fork, modified 2026-11-18. [GPLv3 or later](docs/manual/General_Public_License__version_3.html) · [Original credits and third-party licenses](docs/manual/Acknowledgments.html)
