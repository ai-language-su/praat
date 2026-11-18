# Praat · Unofficial Windows fork

**OpenAI Codex developed the changes in this fork.**

### Added since official Praat 7.0.03

- **Autosave and recovery** with five-minute backups by default, snapshot previews, and reversible removal.
- **Named projects** that restore open editors, zoom, and selections. Relink missing recordings or package a project with its audio.
- **Batch Analysis** with reusable presets, previews, and CSV export for duration, F1/F2, and F0.
- **Corpus annotation tools** to find labels across TextGrids, jump to a match, and reuse tiers and allowed labels.
- **Windows usability** with Search Praat, contextual help, themes, button icons, drag-and-drop, and double-click to edit.

### Get `Praat.exe` on Windows

1. Download [Praat-Windows-x64v3.zip](https://github.com/ai-language-su/praat/releases/latest/download/Praat-Windows-x64v3.zip).
2. Right-click the ZIP in File Explorer and choose **Extract All**.
3. Open the extracted folder and double-click **Praat.exe**. No installer is needed.

The download requires Windows 10 or 11 and an x64 processor with x86-64-v3 support.

If you cloned the repository or downloaded the source ZIP, follow the [MSYS2 setup instructions](HOW_TO_BUILD_ONE.md#1-developing-praat-for-windows). Then run this command from the repository folder in the **MSYS2 MINGW64** shell. It creates `Praat.exe` in that folder.

```sh
make PRAAT_ARCH=x64v3 PRAAT_COMPILER=gcc -j4
```

[Releases and matching source](https://github.com/ai-language-su/praat/releases/latest) · [Official Praat](https://github.com/praat/praat.github.io)

Unofficial fork, modified 2026-11-18. [GPLv3 or later](docs/manual/General_Public_License__version_3.html) · [Original credits and third-party licenses](docs/manual/Acknowledgments.html)
