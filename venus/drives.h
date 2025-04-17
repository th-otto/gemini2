/*
 * @(#) Gemini\drives.h
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 */


#ifndef G_drives__
#define G_drives__

void ExecDriveInfo (char *line);
int WriteDriveInfos (MEMFILEINFO *mp, char *buffer);

void DriveInstDialog (const char *pathname);

void UpdateDrives (int structure_changed, int redraw);
void RegisterDrivePosition (char drive, word x, word y);

void DriveWasRemoved (char drive);
void DriveHasNewShortcut (char drive, int shortcut);
void DriveHasNewName (char drive, const char *name);

#endif