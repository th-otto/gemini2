/*
 * @(#) MParse\MakeTree.h
 * @(#) Stefan Eissing, 25. April 1992
 *
 */

#ifndef __MakeTree__
#define __MakeTree__

#include "partypes.h"


#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

TREEINFO *MakeForkInfo (MGLOBAL *M, int flags, TREEINFO *tree);
TREEINFO *MakeListInfo (MGLOBAL *M, int typ, TREEINFO *left, 
	TREEINFO *right);
NAMEINFO *MakeNameInfo (MGLOBAL *M, const char *name);

/* Die unten stehenden Funktionen geben den Speicherplatz der
 * Åbergebenen Strukturen wieder frei. Die ersten drei Funtionen
 * geben immer einen NULL-Pointer zurÅck. Dies wird im Parser aus
 * GrÅnden des einfacheren Abbruchs "return FreeBlaBla ()" gemacht,
 * hat aber sonst keine tiefere Bedeutung.
 */
void *FreeTreeInfo (MGLOBAL *M, TREEINFO *tree);
void *FreeIoInfo (MGLOBAL *M, IOINFO *tree);
void *FreeRegInfo (MGLOBAL *M, REGINFO *tree);
void *FreeFuncInfo (MGLOBAL *M, FUNCINFO *func);
void *FreeNameInfo (MGLOBAL *M, NAMEINFO *n);
void *FreeSubstListInfo (MGLOBAL *M, SUBSTLISTINFO *list);
void *FreeParInfo (MGLOBAL *M, PARINFO *par);
void *FreeWordInfo (MGLOBAL *M, WORDINFO *w);
void *FreeForkInfo (MGLOBAL *M, FORKINFO *f);
void *FreeCommandInfo (MGLOBAL *M, COMMANDINFO *cmd);
void *FreeIfInfo (MGLOBAL *M, IFINFO *i);
void *FreeWhileInfo (MGLOBAL *M, WHILEINFO *w);
void *FreeForInfo (MGLOBAL *M, FORINFO *f);
void *FreeCaseInfo (MGLOBAL *M, CASEINFO *c);
void *FreeCaseInfo (MGLOBAL *M, CASEINFO *c);
void *FreeListInfo (MGLOBAL *M, LISTINFO *list);

#endif /* __MakeTree__ */

