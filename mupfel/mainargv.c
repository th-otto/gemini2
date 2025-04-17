/*
	@(#)dosix/mainargv.c
	
	Julian F. Reschke, 13. Januar 1994
	
	Startup fuer Atari-ARGV
	Copyright (c) J. Reschke 1990..94
	
	Anwendung: im Hauptprogramm
	'main' durch 'argvmain' ersetzen
*/

#include <ctype.h>
#include <ext.h>
#include <tos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef MAKELIB
#	define MALLOC malloc
#	define MFREE free
#else
#	define MALLOC Malloc
#	define MFREE Mfree
#endif

#define MIN_ARGV_COUNT	4

extern int argvmain (int, char **, char **);

#define MUPFEL 1

#ifndef MUPFEL
char **environ;
static DTA _mydta;
#endif

static char *
get_argv (void)
{
	char *c = _BasPag->p_env;
	
	while (*c)
	{
		if (!strncmp (c, "ARGV=", 5))
			return c + 5;
		else
			c += strlen (c) + 1;
	}
	
	return NULL;
}

int
main (int argc, char *argv[], char *envp[])
{
	char **myargv;
	extern BASPAG *_BasPag;
	char *min_argv[MIN_ARGV_COUNT];
	char *env;
	char *startpar;
	char *argv_value;
	int count;
	int i;

#ifndef MUPFEL
	if (stderr->Handle != 2) {/* schon passiert? */
		stderr->Handle = 2;             /* umsetzen! */
		if (!getenv ("STDERR") && isatty (2))
			Fforce (2, -1);
	}
	
	/* Immer MiNT-Domain */
	Pdomain (1);
	
	environ = envp;

	/* Damit der erste Fsfirst() nicht die
	Kommandozeile �berschreibt */	
	Fsetdta (&_mydta);
#endif

	/* Flag fuer Verwendung von ARGV */
	if (_BasPag->p_cmdlin[0] != 127)
		return argvmain (argc, argv, envp);

	/* Zeiger auf Env-Var merken */
	argv_value = env = get_argv ();
	if (!env) return argvmain (argc, argv, envp);
		
	/* alle weiteren envp's l�schen */
	i = 0;
	while (strncmp (envp[i], "ARGV=", 5)) i++;
	envp[i] = NULL;	

	/* zu argv[0] gehen */
	while (*env++);
	
	/* auf extended ARGV testen */
	if (strncmp (argv_value, "NULL:", 5))
		argv_value = NULL;
	else
		argv_value += 5;
		
	/* Parameterstart */
	startpar = env;
	
	/* Parameter z�hlen */
	count = 0;
	while (*env) {
		count++;
		while (*env++);
	}

	/* Speicher fuer neuen Argument-Vektor */
	if (count < MIN_ARGV_COUNT)
		myargv = min_argv;
	else
		myargv = MALLOC ((count+1)*sizeof (char *));

	env = startpar;
	
	count = 0;
	while (*env) {
		myargv[count++] = env;
		while (*env++);
	}
	
	myargv[count] = NULL;
	
	/* Leere Argumente vernichten */
	if (argv_value)
	{
		while (*argv_value) {
			unsigned int idx = 0;
			char *s = argv_value;
			
			for (; *s && *s != ',' ;) {
				if (! isdigit (*s))
					goto bail_out;
				
				idx *= 10;
				idx += *s++ - '0';
			}
			
			if (*s == ',') s += 1;
			argv_value = s;

			if (idx < count)
				myargv[idx][0] = '\0';
			else
				goto bail_out;
		}
	}

bail_out:

	/* und ...argvmain() starten */
	count = argvmain (count, myargv, envp);	

	if (myargv != min_argv)
		MFREE (myargv);

	return count;
}
