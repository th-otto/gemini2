/*
 * @(#) Gemini\init.h
 * @(#) Stefan Eissing, 17. September 1991
 *
 * description: Header File for init.c
 */


#ifndef G_INIT__
#define G_INIT__

int InitVDI (word v_handle);
void ExitVDI (void);

int GetTrees (void);
void InitNewDesk (void);
void InitWindows (void);
void InitDisk (void);
word open_vwork (void);
int MupVenus (char *autoexec);

#endif /* !G_INIT__ */