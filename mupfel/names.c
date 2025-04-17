/*
 * @(#) Mupfel\Names.c
 * @(#) Stefan Eissing, 14. MÑrz 1994
 *
 * ext. ARGV by Julian F. Reschke, 12. Januar 1994
 *
 * jr 25.6.1996
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "duptree.h"
#include "hash.h"
#include "ioutil.h"
#include "partypes.h"
#include "parsinpt.h"
#include "maketree.h"
#include "names.h"

/* internal texts
 */
#define NlsLocalSection "M.names"
enum NlsLocalText{
NA_NOMEM,	/*nicht genug Speicher, um %s zu setzen\n*/
NA_READONLY,	/*Variable %s ist schreibgeschÅtzt\n*/
NA_ILLEGAL,	/*UngÅltiger Variablenname %s\n*/
};


#define DEFAULT_IFS	" \t\n\r"

const char *GetPATHValue (MGLOBAL *M)
{
	return NameInfoValue (M->path_name);
}

const char *GetIFSValue (MGLOBAL *M)
{
	return M->ifs_name->val ? M->ifs_name->val : DEFAULT_IFS;
}

const char *GetPS1Value (MGLOBAL *M)
{
	return NameInfoValue (M->ps1_name);
}

const char *GetPS2Value (MGLOBAL *M)
{
	return NameInfoValue (M->ps2_name);
}

const char *GetPS4Value (MGLOBAL *M)
{
	return NameInfoValue (M->ps4_name);
}

/* jr: Funktion, die die Werte von autoset-Variablen aktualisiert */

static int
update_var (MGLOBAL *M, NAMEINFO *n, const char *newbuf)
{
	char *cp;

	if (strlen (n->val) >= strlen (newbuf)) {
		strcpy ((char *) n->val, newbuf);
		return TRUE;
	}
	
	cp = StrDup (&M->alloc, newbuf);
	if (! cp) return FALSE;
	
	mfree (&M->alloc, n->val);
	n->val = cp;
	
	return TRUE;	
}

void
UpdateAutoVariable (MGLOBAL *M, NAMEINFO *n)
{
	char buffer[256];	/* enough ;-) */

	if (!strcmp (n->name, "LINENO"))
		sprintf (buffer, "%ld", ParsLineCount (M));
	else if (!strcmp (n->name, "SECONDS"))
		sprintf (buffer, "%ld", (GetHz200Timer () - M->times) / 200);
	else if (!strcmp (n->name, "COLUMNS"))
		sprintf (buffer, "%d", M->tty.columns);
	else if (!strcmp (n->name, "ROWS") || !strcmp (n->name, "LINES"))
		sprintf (buffer, "%d", M->tty.rows);
	else
		return;
		
	update_var (M, n, buffer);
}

int IsLegalEnvName (const char *name)
{
	if (IsEnvChar (*name++))
	{
		while (*name)
		{
			if (!IsEnvChar (*name))
				return FALSE;
			
			++name;
		}
		return TRUE;
	}

	return FALSE;
}

static int isReservedNameInfo (MGLOBAL *M, NAMEINFO *n)
{
	return ((n == M->path_name)
		|| (n == M->ifs_name)
		|| (n == M->ps1_name)
		|| (n == M->ps2_name)
		|| (n == M->ps4_name)
		);
}

static void setReservedNameInfo (MGLOBAL *M, NAMEINFO *n)
{
	if (!strcmp (n->name, "PATH"))
		M->path_name = n;
	else if (!strcmp (n->name, "IFS"))
		M->ifs_name = n;
	else if (!strcmp (n->name, "PS1"))
		M->ps1_name = n;
	else if (!strcmp (n->name, "PS2"))
		M->ps2_name = n;
	else if (!strcmp (n->name, "PS4"))
		M->ps4_name = n;
}

/* Finde den NAMEINFO von <name>, wenn <name> nicht gefunden wurde,
   wird NULL zurÅckgegeben. <lastnext> wird mit der Adresse des
   zuletzt untersuchten <n->next> Zeigers besetzt. An diesen kann
   dann eine neue Struktur eingefÅgt werden. Es wird davon
   ausgegangen, daû die Liste alphabetisch sortiert ist. */

static
NAMEINFO *findNameInfo (MGLOBAL *M, const char *name, NAMEINFO ***lastnext)
{
	NAMEINFO *n;
	
	*lastnext = &M->name_list;
	n = M->name_list;
	
	while (n)
	{
		int cmp = strcmp (n->name, name);
		
		if (!cmp)
			return n;
		else if (cmp > 0)
			break;
			
		*lastnext = &n->next;
		n = n->next;
	}

	return NULL;
}


/* jr: Seiteneffekt: autoset-Variablen werden aktualisiert */

NAMEINFO *GetNameInfo (MGLOBAL *M, const char *name)
{
	NAMEINFO **dummy;
	NAMEINFO *n = findNameInfo (M, name, &dummy);
	
	if (n && n->flags.autoset) UpdateAutoVariable (M, n);
	
	return n;
}

/* Sucht die NAMEINFO fÅr <name>. Falls nicht vorhanden, wird eine
   neue NAMEINFO fÅr <name> angelegt. Nur im Fall, daû kein Speicher
   zur VerfÅgung steht, wird NULL zurÅckgeliefert. */

NAMEINFO *
MakeOrGetNameInfo (MGLOBAL *M, const char *name)
{
	NAMEINFO *n, **lastnext;
	
	if (!IsLegalEnvName (name))
	{
		eprintf (M, NlsStr(NA_ILLEGAL), name);
		return NULL;
	}
	
	n = findNameInfo (M, name, &lastnext);

	if (!n)
	{
		if ((n = MakeNameInfo (M, name)) == NULL)
		{
			eprintf (M, NlsStr(NA_NOMEM), name);
			return NULL;
		}
		n->next = *lastnext;
		*lastnext = n;	
	}
	else if (n->flags.autoset)
		UpdateAutoVariable (M, n);
	
	return n;
}


/* Setzt die NAMEINFO <n> auf den Wert <value>, wobei vorhandene
   Werte freigegeben werden. Steht nicht genug Speicherplatz zur
   VerfÅgung, wird FALSE zurÅckgeliefert und n wird unverÑndert
   gelassen. */

int
SetNameInfo (MGLOBAL *M, NAMEINFO *n, const char *value)
{
	char *v;
	
	if (value)
	{
		v = StrDup (&M->alloc, value);
		if (!v) return FALSE;
	}
	else
		v = NULL;
		
	if (n->flags.function)		/* Funktionen werden freigegeben */
	{
		FreeFuncInfo (M, n->func);
		n->func = NULL;
		n->flags.function = FALSE;
	}
	
	if (n->val)					/* Ebenso alte Werte */
		mfree (&M->alloc, n->val);
	
	n->val = v;					/* Neuen Wert setzen */
	
	if (n == M->path_name)
	{
		/* PATH wurde geÑndert. Das Hashing muû darÅber
		   benachrichtigt werden! */

		ClearHash (M);
	}

	return TRUE;
}

int
InstallName (MGLOBAL *M, const char *name, const char *value)
{
	NAMEINFO *n;
	
	n = MakeOrGetNameInfo (M, name);

	if (!n) return FALSE;
	
	if (n->flags.readonly)
	{
		eprintf (M, NlsStr(NA_READONLY), name);
		return FALSE;
	}

	/* Variable wurde manuell gesetzt */
	n->flags.autoset = 0;

	return SetNameInfo (M, n, value);	/* Wert setzen */
}

int InstallFunction (MGLOBAL *M, FUNCINFO *func)
{
	NAMEINFO *n;
	
	n = MakeOrGetNameInfo (M, func->name->name);

	if (n == NULL) return FALSE;
		
	if (n->flags.readonly)
	{
		eprintf (M, "%s ist schreibgeschÅtzt (readonly)!\n",
			func->name->name);
		return FALSE;
	}
		
	/* Die Bourne-Shell lîscht beim Installieren einer
	   Funktion auch ererbte Variablen. Waaarum? */
	   
	if (n->val) free (n->val);
	n->flags.exported = FALSE;
		
	if (n->flags.function)
		FreeFuncInfo (M, n->func);

	n->flags.function = TRUE;
	
	n->func = DupFuncInfo (M, func);
	if (func && !n->func) return FALSE;

	return TRUE;
}


/* jr: setze eine Auto-Set-Variable. `len' ist die LÑnge des
   Feldes. Wenn preserve_old gesetzt ist, wird ein evtl
   geerbter Wert weiterbenutzt */

int
setAutoName (MGLOBAL *M, const char *name, size_t len, int preserve_old)
{
	NAMEINFO *n = MakeOrGetNameInfo (M, name);
	char *tmpstr = mmalloc (&M->alloc, len);
	
	if (tmpstr && n)
	{
		tmpstr[0] = '\0';

		/* jr: ggfs nicht setzen. Reichen die Tests? */
		
		if (n->flags.function || n->flags.readonly)
		{
			mfree (&M->alloc, tmpstr);
			return FALSE;
		}
		
		if (preserve_old && n->val && strlen (n->val) < len)
		{
			strcpy (tmpstr, n->val);
			mfree (&M->alloc, n->val);
			n->val = NULL;
			
			/* jr: Special case: die Werte fÅr LINES und COLUMNS
			   mÅssen in die TTY-Struktur eingetragen werden */
			   
			if (!strcmp (name, "LINES"))
				M->tty.rows = atoi (tmpstr);

			if (!strcmp (name, "COLUMNS"))
				M->tty.columns = atoi (tmpstr);
		}
		
		if (n->val)
		{	
			mfree (&M->alloc, tmpstr);
			return FALSE;
		}
		
		n->val = tmpstr;
		n->flags.autoset = 1;

		return TRUE;
	}

	if (tmpstr) mfree (&M->alloc, tmpstr);
	return FALSE;
}

int
SetEnvName (MGLOBAL *M, const char *name, const char *value)
{
	NAMEINFO *n = MakeOrGetNameInfo (M, name);

	if (n)
	{
		if (n->flags.function || n->flags.readonly)
/* jr: xxx sollte das vielleicht FALSE sein? */
			return TRUE;
		else
			setReservedNameInfo (M, n);

		n->flags.exported = 1;
		n->flags.autoset = 0;

		/* jr: value zeigt auf etwas im geerbten Environment oder
		auf eine Konstante. Wir brauchen aber etwas alloziertes,
		was wieder freigegeben werden kann */

		if (n->val) mfree (&M->alloc, n->val);
		n->val = StrDup (&M->alloc, value);
/* jr: xxx ist dieser Ausgang korrekt? Was ist dann mit der Struktur? */
		if (!n->val) return FALSE;

		return TRUE;
	}
	
	return FALSE;
}

int
ExportName (MGLOBAL *M, const char *name, char *value)
{
	NAMEINFO *n;
	
	n = MakeOrGetNameInfo (M, name);
	if (n)
	{
		if ((n->flags.readonly && value) || n->flags.function)
			return FALSE;

		n->flags.exported = 1;

		if (value)
		{
			/* jr: Der User hat explizit einen Wert gesetzt */
			n->flags.autoset = 0;
			return SetNameInfo (M, n, value);
		}

		return TRUE;
	}
	else
		return FALSE;
}

int
ReadonlyName (MGLOBAL *M, const char *name, char *value)
{
	NAMEINFO *n = MakeOrGetNameInfo (M, name);
	
	if (n)
	{
		int retcode = TRUE;
		
		if ((n->flags.readonly && value) || n->flags.function)
			return FALSE;

		if (value)
			retcode = SetNameInfo (M, n, value);

		n->flags.readonly = 1;
		n->flags.autoset = 0;

		return retcode;
	}
	else
		return FALSE;
}


/* jr: xxx sollten autosets hier neu initialisiert werden? */

int
UnsetNameInfo (MGLOBAL *M, const char *name, int which)
{
	NAMEINFO *n, **lastnext;
	
	n = findNameInfo (M, name, &lastnext);
	
	if (!n || n->flags.readonly)
		return FALSE;
	
	if (n->flags.function && !(which & UNSET_FUNCTION))
		return FALSE;
	if (!n->flags.function && !(which & UNSET_VARIABLE))
		return FALSE;
	
	if (isReservedNameInfo (M, n))
		return SetNameInfo (M, n, NULL);
	
	*lastnext = n->next;
	FreeNameInfo (M, n);

	return TRUE;	
}

static void overrideNames (MGLOBAL *M, int var_argc, 
	const char **var_argv)
{
	int i;
	
	for (i = 0; i < var_argc; ++i)
	{
		NAMEINFO **dummy, *n;
		char *scan;
		
		scan = strchr (var_argv[i], '=');
		if (scan)
		{
			*scan = '\0';
			n = findNameInfo (M, var_argv[i], &dummy);
			*scan = '=';
			
			if (n) n->flags.overridden = 1;
		}
	}
}

static void
undoOverride (MGLOBAL *M)
{
	NAMEINFO *n;
	
	for (n = M->name_list; n; n = n->next)
		n->flags.overridden = 0;
}

char *
MakeEnvironment (MGLOBAL *M, int withArgv, int argc, 
	const char **argv, const char *program_name, int var_argc,
	const char **var_argv, size_t *total_size)
{
	NAMEINFO *n;
	int i;
	size_t len = 0L;
	char *env, *cp;
	int empty_args = 0;
	
	overrideNames (M, var_argc, var_argv);
	
	for (n = M->name_list; n; n = n->next)
	{
		if (n->flags.exported && !n->flags.overridden)
		{
			if (n->flags.autoset) UpdateAutoVariable (M, n);
			
			len += strlen (n->name) + 2;
			if (n->val) len += strlen (n->val);
		}
	}
	
	for (i = 0; i < var_argc; ++i)
		len += strlen (var_argv[i]);
	
	if (withArgv)
	{
		int i;
		
		for (i = 1; i < argc; ++i)
		{
			size_t sl = strlen (argv[i]);
		
			len += sl;
			if (!sl)
			{
				empty_args++;
				len++;
			}
		}
		
		len += strlen (program_name) + 1;
		len += 6L + (long)i;	/* account for ARGV= and zeroes */

		/* if empty args are present, use extended argv:
		
		ARGV=NULL:idx,idx,... */
		
		if (empty_args) len += 5 + 8 * empty_args;
	}

	*total_size = len + 1;
	
	/*
	 * Get enough memory. 1K added in case the child process doesn't
	 * maintain his own copy
	 */
	len += 1024L;
	
	cp = env = mmalloc (&M->alloc, len);
	if (!env)
	{
		if (var_argc) undoOverride (M);
		return NULL;
	}
	
	memset (env, 0, len);
	
	/* Kopiere die exportierten Variablen. Und zwar nur die,
	die nicht 'overridden' sind */
	
	for (n = M->name_list; n; n = n->next)
	{
		if (n->flags.exported && !n->flags.overridden)
			cp += 1 + sprintf (cp, "%s=%s", n->name,
				n->val ? n->val : "");
	}
	
	/* Kopiere die restlichen Variablen */
	for (i = 0; i < var_argc; i++)
		cp += 1 + sprintf (cp, "%s", var_argv[i]);
	
	/* Wenn wir ARGV benutzen, hÑnge ein "ARGV=" and <argv> an.
	 */ 
	if (withArgv)
	{
		int i;
		
		if (!empty_args)
			cp += 1 + sprintf (cp, "%s", "ARGV=");
		else
		{
			int need_comma = 0;
			int j;
			
			cp += sprintf (cp, "%s", "ARGV=NULL:");
			
			for (j = 0; j < argc; j++)
			{
				if (argv[j][0] == '\0')
				{
					if (need_comma) *cp++ = ',';
					
					cp += sprintf (cp, "%d", j);
					need_comma = 1;
				}
			}
			
			*cp++ = '\0';
		}

		cp += 1 + sprintf (cp, "%s", program_name);

		for (i = 1; i < argc; ++i)
		{
			const char *c = argv[i];
			
			if (*c == '\0') c = " ";	/* empty args as ' ' */
			cp += 1 + sprintf (cp, "%s", c);
		}
	}
	/* Schlieûe des Environment mit einer 0 ab
	 */
	*cp = '\0';
	
	if (var_argc) undoOverride (M);
	
	return env;
}

/* Untersucht das Environment <penv> und trÑgt die dort liegenden
   Wertepaare in die M->name_list ein. Bereits vorhandene Werte in
   M->name_list werden nicht Åberdeckt. War nicht genug Speicher
   vorhanden, wird FALSE zurÅckgeliefert. */

int
InitNames (MGLOBAL *M, char *penv)
{
	M->path_name = M->ifs_name = M->ps1_name = M->ps2_name = 
	M->ps4_name = NULL;
	
	while (penv && *penv)
	{
		char *scan;
		
		if (strstr (penv, "ARGV=") == penv)
		{
			/* Ende der Fahnenstange. Kann nur letzter Eintrag im
			   Environment sein. */
			penv = NULL;
			continue;
		}
		
		if ((scan = strchr (penv, '=')) != NULL)
		{
			/* Wir ignorieren absichtlich den Returnwert von
			   setEnvname, da wir nicht zwischen Speichermangel und
			   falschen Namen entscheiden kînnen, deshalb aber nicht
			   die Installation der Shell abbrechen wollen. */

			*scan = '\0';
			if (strcmp (penv, "xArg")) SetEnvName (M, penv, scan+1);
			*scan = '=';
		}
		
		while (*penv)
			++penv;
		++penv;
	}
	
	if (!M->path_name || !M->path_name->val
		|| M->path_name->val[0] == '\0')
	{
		if (!SetEnvName (M, "PATH", "."))
			return FALSE;
	}

	if (!M->ifs_name || !M->ifs_name->val)
	{
		if (!SetEnvName (M, "IFS", DEFAULT_IFS))
			return FALSE;
		M->ifs_name->flags.exported = 0;
	}

	if (!M->ps1_name)
		if (!SetEnvName (M, "PS1", "$"))
			return FALSE;

	if (!M->ps2_name)
		if (!SetEnvName (M, "PS2", ">"))
			return FALSE;

	if (!M->ps4_name)
		if (!SetEnvName (M, "PS4", "+ "))
			return FALSE;

	/* jr: PPID setzen */
	{
		long ppid = Pgetppid ();
		char buf[15];
		
		if (ppid == EINVFN)
		{
			BASPAG *p = getactpd ();
			ppid = (long)p->p_parent;
		}
	
		sprintf (buf, "%ld", ppid);
		SetEnvName (M, "PPID", buf);
	}

	/* jr: lokale Autoset-Variablen setzen */

	setAutoName (M, "LINENO", 15, 0);
	setAutoName (M, "SECONDS", 15, 0);

	/* jr: Autoset-Variablen, die ggfs exportiert werden. */

	setAutoName (M, "ROWS", 15, 1);
	ExportName (M, "ROWS", NULL);
	setAutoName (M, "LINES", 15, 1);
	ExportName (M, "LINES", NULL);
	setAutoName (M, "COLUMNS", 15, 1);
	ExportName (M, "COLUMNS", NULL);

	return TRUE;
}

/* Diese Funktion kopiert den Namespace des ShellLevels <old> in
   den Level <new>. Ist nicht genug Speicher vorhanden, dann wird
   FALSE zurÅckgeliefert. */
   
int
ReInitNames (MGLOBAL *old, MGLOBAL *new)
{
	NAMEINFO *list = old->name_list;
	NAMEINFO **nl = &new->name_list;
	
	new->name_list = new->path_name = new->ifs_name = 
	new->ps1_name = new->ps2_name = new->ps4_name = NULL;

	while (list)
	{
		NAMEINFO *n;
		
		n = mmalloc (&new->alloc, sizeof (NAMEINFO));
		if (!n) return FALSE;
		
		*n = *list;
		
		/* Dupliziere den Namen der Variable
		 */
		n->name = StrDup (&new->alloc, list->name);
		if (!n->name)
		{
			mfree (&new->alloc, n);
			return FALSE;
		}
		
		if (n->flags.function && list->func)
		{
			n->func = DupFuncInfo (new, list->func);
			if (!n->func)
			{
				mfree (&new->alloc, n->name);
				mfree (&new->alloc, n);
				
				return FALSE;
			}
		}
		else if (list->val)
		{
			n->val = StrDup (&new->alloc, list->val);
			if (!n->val)
			{
				mfree (&new->alloc, n->name);
				mfree (&new->alloc, n);
				return FALSE;
			}
		}
		
		if (!n->flags.function) setReservedNameInfo (new, n);
			
		n->next = NULL;
		*nl = n;
		nl = &n->next;
		
		list = list->next;
	}
	
	return TRUE;
}


/* Gibt die bei <list_start> beginnende Liste wieder frei. */

static void
freeNameInfoList (MGLOBAL *M, NAMEINFO *list_start)
{
	while (list_start)
	{
		NAMEINFO *next = list_start->next;
		
		FreeNameInfo (M, list_start);
		list_start = next;
	}
}

void
ExitNames (MGLOBAL *M)
{
	freeNameInfoList (M, M->name_list);
	M->name_list = NULL;
}

void
CreateAutoVar (void *M, const char *name, size_t size)
{
	setAutoName (M, name, size, 1);
	ExportName (M, name, NULL);
}
