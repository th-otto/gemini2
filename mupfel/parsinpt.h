/*
 * @(#) MParse\ParsInpt.h
 * @(#) Stefan Eissing, 16. Januar 1992
 *
 * jr 21.4.1995
 */


#ifndef __ParsInpt__
#define __ParsInpt__

void ParsFromStdin (MGLOBAL *M, PARSINPUTINFO *save);
void ParsFromString (MGLOBAL *M, PARSINPUTINFO *save, 
	const char *line);
long ParsFromFile (MGLOBAL *M, PARSINPUTINFO *save, 
	const char *file_name);
void ParsFromArgv (MGLOBAL *M, PARSINPUTINFO *save, 
	int argc, const char **argv);

void ResetParsInput (MGLOBAL *M);
int ParsEOFReached (MGLOBAL *M);
size_t ParsLineCount (MGLOBAL *M);

const char *GetParsInputLine (MGLOBAL *M, int *broken);
int PushBackInputLine (MGLOBAL *M, const char *line);


void ParsRestoreInput (MGLOBAL *M, PARSINPUTINFO *saved);

#endif /* __ParsInpt__ */