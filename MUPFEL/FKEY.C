/*
 * @(#) Mupfel\fkey.c
 * @(#) Stefan Eissing & Gereon Steffens, 28. Mai 1992
 *
 *  -  internal "fkey" function
 *
 * jr 27.11.94
 */
 
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "mglob.h"

#include "chario.h"
#include "fkey.h"
#include "scan.h"

/* internal texts
 */
#define NlsLocalSection "M.fkey"
enum NlsLocalText{
OPT_fkey,	/*[taste [text]]*/
HLP_fkey,	/*Funktionstasten setzen/l敗chen/anzeigen*/
FK_ILLKEY,	/*Ung〕tige Funktionstaste*/
};

/* Wandle eine gedr…kte Funktionstaste in deren Nummer um.
 * Wenn <key_no> keine Funktionstaste ist, wird -1 zur…k-
 * geliefert.
 */
int ConvKey (int key_no)
{
	if (key_no >= F1 && key_no <= F10)
		return key_no - F1;

	if (key_no >= SF1 && key_no <= SF10)
		return key_no - SF1 + 10;
		
	return -1;
}


/* Belege die Funktionstaste mit der Nummer <key_no> mit dem
 * Text <text>. Bei Mi柩ingen wird FALSE zur…kgeliefert.
 */
int SetFKey (MGLOBAL *M, int key_no, char *text)
{
	if (key_no >= 0 && key_no < FKEYMAX)
	{
		if (M->tty.fkeys[key_no] != NULL)
			mfree (&M->alloc, M->tty.fkeys[key_no]);
			
		if (strlen (text) > 0)
			M->tty.fkeys[key_no] = StrDup (&M->alloc, text);
		else
			M->tty.fkeys[key_no] = NULL;
		
		return TRUE;
	}
	else
	{
		eprintf (M, NlsStr(FK_ILLKEY));
		return FALSE;
	}
}


/* Liefert den Text, mit dem die Funktionstaste mit der Nummer
 * <key_no> belegt ist, zur…k. Im Fehlerfall ist dies der NULL-
 * Pointer, bei unbelegten Tasten ein leerer String.
 */
const char *GetFKey (MGLOBAL *M, int key_no)
{
	if (key_no >= 0 && key_no < FKEYMAX)
		return M->tty.fkeys[key_no] != NULL ? 
			M->tty.fkeys[key_no] : "";
	else
		return NULL;
}


/* Liefert einen String mit dem Namen der Funktionstaste mit der
 * Nummer <key> zur…k.
 */
static const char *keyName (int key)
{
	static const char *kname[] =
	{
		"F1",  "F2",  "F3",  "F4",  "F5",
		"F6",  "F7",  "F8",  "F9",  "F10",
		"SF1", "SF2", "SF3", "SF4", "SF5",
		"SF6", "SF7", "SF8", "SF9", "SF10"
	};
	
	return (key >= 0 && key < FKEYMAX) ? kname[key] : "??";
}


/* Gegenst…k: Wandelt den Text <name> in eine Funktionstasten-
 * nummer um. Im Fehlerfall wird -1 zur…kgeliefert.
 */
static int getKeyNo (char *name)
{
	int key_no, i;
	
	strupr (name);
	
	if ((key_no = atoi (name)) == 0)
	{
		if (*name == 'F')
		{
			key_no = atoi (name + 1) - 1;
			if (key_no >= 0 && key_no < FKEYMAX)
				return key_no;
		}
		
		for (i = 0; i < FKEYMAX; ++i)
		{
			if (!strcmp (name, keyName (i)))
				return i;
		}
		
		return -1;
	}
	else
		return key_no - 1;
}


/* Gibt die Belegung der Funktionstasten aus.
 */
void PrintFKeys (MGLOBAL *M)
{
	int i;
	
	for (i = 0; i < FKEYMAX && !intr (M); ++i)
	{
		if (M->tty.fkeys[i] != NULL)
			mprintf (M, "%4s: %s\n", keyName (i), M->tty.fkeys[i]);
	}
}


/* Internes Kommando <fkey>, das Funktionstasten anzeigen und
 * belegen kann.
 */
int m_fkey (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_fkey), 
			NlsStr(HLP_fkey));
	
	switch (argc)
	{
		case 1:
			PrintFKeys (M);
			break;
		case 2:
			SetFKey (M, getKeyNo (argv[1]), "");
			break;
		case 3:
			SetFKey (M, getKeyNo (argv[1]), argv[2]);
			break;
		default:
			return PrintUsage (M, argv[0], NlsStr(OPT_fkey));
	}
	
	return 0;
}
