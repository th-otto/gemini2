/*
 * @(#) Gemini\dispatch.h
 * @(#) Stefan Eissing, 12. November 1994
 *
 * description: Header File for dispatch.c
 */


#ifndef G_dispatch__
#define G_dispatch__


void doDclick (WindInfo *wp, OBJECT *tree, word found_object, 
	word kstate);

void simDclick (word kstate);

int StartFile (WindInfo *wp, word fobj, word showpath, char *label,
	char *path, char *name, ARGVINFO *args, word kbd_state,
	word *retcode);

int CheckInteractiveForAccess (char **name, char *label, int isFolder,
			int objnr, int *cancelled);

#endif	/* !G_dispatch__ */