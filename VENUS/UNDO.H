/*
 * @(#) Gemini\undo.h
 * @(#) Stefan Eissing, 27. Juli 1991
 *
 * description: Header File for undo.c
 */


#ifndef G_undo
#define G_undo

void storeWOpenUndo (word open, WindInfo *wp);
void storeWMoveUndo (WindInfo *wp);
void storePathUndo (WindInfo *wp);
void UndoFolderVanished (const char *path);

void doUndo (void);
void clearUndo (void);

#endif	/* !G_undo */