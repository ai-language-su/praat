#ifndef _praat_session_h_
#define _praat_session_h_

// Periodic and normal-exit session backups. Never overwrite imported data files.
void praat_session_prefs ();
bool praat_session_isEnabled ();
void praat_session_setEnabled (bool enabled);
bool praat_session_isActive ();
bool praat_session_autosaveEnabled ();
integer praat_session_autosaveMinutes ();
integer praat_session_backupsToKeep ();
integer praat_session_diskLimitMiB ();
autostring32 praat_session_storageSummary ();
void praat_session_setAutosaveSettings (bool enabled, integer minutes, integer backups, integer storageLimit = 0);
// Windows: start a wake-up timer, then poll only from the outer application loop.
// Nested message loops must never capture partly modified data.
void praat_session_startAutosave ();
void praat_session_stopAutosave ();
void praat_session_pollAutosave ();
void praat_session_save ();
void praat_session_restore ();
void praat_session_choose (bool atStartup);
void praat_project_open (MelderFile file, bool askToRelink);
void praat_project_new ();
bool praat_project_save ();
void praat_project_saveAs (MelderFile file, bool overwriteConfirmed);
void praat_project_package (MelderFile file, bool overwriteConfirmed);

// Also used by the integration test with a private preferences directory.
void praat_session_saveToFolder (MelderFolder folder);
bool praat_session_restoreFromFolder (MelderFolder folder);

#endif
