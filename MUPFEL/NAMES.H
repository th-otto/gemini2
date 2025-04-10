/*
 * @(#) MParse\Names.h
 * @(#) Stefan Eissing, 14. Februar 1993
 *
 * jr 7.8.94
 */


#ifndef __Names__
#define __Names__

#include "partypes.h"


/* Untersucht das Environment <env> und tr„gt die dort liegenden
 * Wertepaare in die M.name_list ein. Bereits vorhandene Werte in
 * M.name_list werden nicht berdeckt.
 * War nicht genug Speicher vorhanden, wird FALSE zurckgeliefert.
 */
int InitNames (MGLOBAL *M, char *penv);
int ReInitNames (MGLOBAL *old, MGLOBAL *new);
void ExitNames (MGLOBAL *M);

int IsLegalEnvName (const char *name);
int InstallName (MGLOBAL *M, const char *name, const char *value);
int InstallFunction (MGLOBAL *M, FUNCINFO *func);

NAMEINFO *GetNameInfo (MGLOBAL *M, const char *name);
NAMEINFO *MakeOrGetNameInfo (MGLOBAL *M, const char *name);


/* Malloc'e ein Environment und flle es
 */
char *MakeEnvironment (MGLOBAL *M, int withARGV, int argc, 
	const char **argv, const char *program_name, 
	int var_argc, const char **var_argv, size_t *total_size);

int SetNameInfo (MGLOBAL *M, NAMEINFO *n, const char *value);

/* Ein Makro zum Lesen des Values einer NAMEINFO
 */
#define NameInfoValue(n)	(n->val ? n->val : "")

/* Einige Funktionen zum schnellen Zugriff auf die Werte
 */
const char *GetPATHValue (MGLOBAL *M);
const char *GetIFSValue (MGLOBAL *M);
const char *GetPS1Value (MGLOBAL *M);
const char *GetPS2Value (MGLOBAL *M);
const char *GetPS4Value (MGLOBAL *M);

/* jr: Funktion, die die Werte von autoset-Variablen aktualisiert */
void UpdateAutoVariable (MGLOBAL *M, NAMEINFO *n);

int ExportName (MGLOBAL *M, const char *name, char *value);
int ReadonlyName (MGLOBAL *M, const char *name, char *value);

#define UNSET_FUNCTION	0x01
#define UNSET_VARIABLE	0x02
int UnsetNameInfo (MGLOBAL *M, const char *name, int which);

#endif /* __Names__ */