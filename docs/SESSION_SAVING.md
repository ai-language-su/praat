# Projects and workspace recovery

Use **Praat > Project > Save project as…** to name a workspace and save it as a
`.PraatProject` file. **Save project** (Ctrl+S on Windows) updates that file.
**Open > Open project…** opens another project, and **Praat > Project > New
project** starts with an empty workspace. With automatic recovery enabled,
Praat takes a snapshot before leaving the current workspace. If that write
fails, the switch is cancelled. With recovery disabled, Praat asks before
closing the current workspace.

A project contains the Objects list in order, including selection and any
in-memory edits, plus the text of all open scripts and notebooks. On Windows it
stores the positions and sizes of these windows, as well as text selections.
Sound, LongSound, TextGrid, Spectrum, Pitch, and Manipulation editors reopen with
their saved zoom and time or frequency selection. TextGrid editors also reopen
with their selected tier and linked audio when that audio remains in Objects.
Praat brings windows back onto an available monitor if the display layout
changed. The saved text reopens without running scripts or notebooks.

LongSound objects refer to their source files. Praat stores paths relative to
the project when those files are under its folder. If **Open project…** cannot
read a linked recording, select its new location in the **Locate recording**
dialog. Projects saved by this build check channel count, sample count, and
sample rate. Once the workspace opens, use **Save project** to keep the new
links. Cancelling the dialog keeps the current workspace open.

Use **Praat > Project > Package project…** to create a ZIP of the current
workspace. The archive contains a `.PraatProject` file and a copy of each linked
LongSound source under `Recordings/`. Ordinary Sound objects and TextGrids are
embedded in the project. Open script and notebook text is included as drafts.
Extract the entire ZIP, then open the extracted `.PraatProject` file with a
Praat build that supports packages. Keep its `Recordings` folder alongside it.
Other files referenced by scripts, such as files used by `include`, need to be
transferred separately.

Other editor types, editor grouping, undo history, and Picture drawings are
outside the saved workspace. Scripts opened within a data editor reopen as
standalone windows. A script's original file is written only by an explicit
save in its editor.

## Recovery history

On Windows the default remains one autosave every five minutes. Normal Quit and
workspace switches also create recovery snapshots when there is work to save.
Praat keeps up to 200 active snapshots by default and targets a 2,048 MiB
recovery storage limit. These backups are separate from project saves and
never overwrite a project or a source file.

When backups are available, a normal launch offers the **Recover workspace**
window. Select the date and project name you want, or choose **Start empty**.
The same list is available under **Open > Recover workspace…**. A recovered
snapshot opens as a separate workspace. Use **Save project as…** to give it a
name or deliberately update a project file. Legacy snapshots still open with the
data they originally saved. If a snapshot cannot be read, the chooser stays
open so that you can select a different date.

The backup directory is `%APPDATA%\Praat\SessionBackups` by default. It follows
`--pref-dir` when supplied. **Praat > Settings > Session saving settings…** shows
recovery usage and lets you change the save interval, snapshot count, and storage
limit. You can also disable periodic autosave while keeping normal-exit backups,
or turn off automatic backups. Explicit project saves and manual recovery remain
available. Batch mode does not write or open recovery snapshots automatically.

Autosave stores identical Sound data once in the `Sounds` subfolder. Edited
versions are stored separately while a snapshot still needs them. Older backups
with embedded Sounds remain readable. **Recently removed** snapshots count
toward the space limit. After a successful save, Praat applies the active
snapshot count limit. If it needs more space, it clears the oldest recently
removed snapshots first, then the oldest active backups. The newly saved
snapshot is kept even if it exceeds the limit by itself. Named projects and
project ZIPs retain their own audio and do not count toward recovery storage.

## Windows build and verification

```sh
export PATH=/mingw64/bin:/usr/bin:$PATH
make PRAAT_ARCH=x64v3 PRAAT_COMPILER=gcc -j4 > /tmp/praat-project-build.log 2>&1
```

```powershell
python -B test/build_session_test.py --build-log C:/msys64/tmp/praat-project-build.log --compiler C:/msys64/mingw64/bin/g++.exe --output C:/msys64/tmp/praat-session-project-test.exe
python -B test/run_session_test.py --test-exe C:/msys64/tmp/praat-session-project-test.exe --praat-exe E:/Projects/Coding/praat/Praat.exe --periodic
```

The test runner uses private temporary data. The optional periodic test waits
for the actual five-minute deadline and tests recovery after forced termination.
The Windows integration checks cover named project saves and updates, mixed
Sound and TextGrid editor restoration, window bounds, view ranges, script
selections, choosing an older snapshot, and starting empty. A snapshot created
with the previous version was also reopened through the current recovery chooser.
Package tests cover ZIP extraction and reopening after moving the project,
relinking and saving a moved recording, rejecting a recording with different
dimensions, and cancelling the picker. Add `--large-package` to test ZIP64
size and offset handling with a recording file over 4 GiB.
