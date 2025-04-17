/*
 * @(#) Gemini\infofile.h
 * @(#) Stefan Eissing, 03. August 1991
 *
 * description: Header File for infofile.c
 */


#ifndef G_INFOFILE__
#define G_INFOFILE__

int ReadInfoFile (const char *inf_name, const char *gin_name, 
	word *tmp);
int WriteInfoFile (const char *inf_name, const char *gin_name, 
	word update);

void ExecConfInfo (word todraw);
int MakeConfInfo (void);

#endif	/* !G_INFOFILE__ */