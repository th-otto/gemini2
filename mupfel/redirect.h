/*
 * @(#) MParse\Redirect.h
 * @(#) Stefan Eissing, 13. September 1992
 */


#ifndef __redirect__
#define __redirect__

#include "partypes.h"

int InitRedirection (MGLOBAL *M, int force_to_con);
int ReInitRedirection (MGLOBAL *M, MGLOBAL *new);

/* Darf nur einmal zu Beginn des Programms aufgerufen werden!
 */
void SetupStdErr (MGLOBAL *M);

void ExitRedirection (MGLOBAL *M);

int isatty (int handle);
int StdoutIsatty (MGLOBAL *M);
int StdinIsatty (MGLOBAL *M);
int StderrIsatty (MGLOBAL *M);

#define StdoutIsConsole(M)	((M)->redir.stream[1].handle == -1)

int OpenPipe (MGLOBAL *M, IOINFO *left, IOINFO *right);
void ClosePipe (MGLOBAL *M, IOINFO *left, IOINFO *right);

int DoRedirection (MGLOBAL *M, IOINFO *io);
int UndoRedirection (MGLOBAL *M, IOINFO *io);

#endif /* __redirect__ */