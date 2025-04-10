/*
	@(#)mainargv.c
	@(#)Julian F. Reschke, 3. Juli 1990
	
	Startup fuer Atari-ARGV
	Copyright (c) J. Reschke 1990
	
	Anwendung: im Hauptprogramm
	'main' durch 'argvmain' ersetzen
*/

#include <tos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int argvmain (int, char **, char **);

#define MIN_ARGV_COUNT	4

int main (int argc, char *argv[], char *envp[])
{
	char **myargv;
	char *minargv[MIN_ARGV_COUNT];
	extern BASPAG *_BasPag;
	char *env;
	char *startpar;
	char *oldline;

	int count = 0;
	int i, malloced = 0;

	/* Anfang der alten Kommandozeile */
	oldline = &_BasPag->p_cmdlin[1];

	/* Flag fuer Verwendung von ARGV */
	if (_BasPag->p_cmdlin[0] != 127)
		return argvmain (argc, argv, envp);

	/* Zeiger auf Env-Var merken */
	env = getenv("ARGV");
	if (!env)
		return argvmain (argc, argv, envp);
		
	/* alle weiteren envp's l”schen */
	i = 0;
	while (envp[i] && strncmp (envp[i], "ARGV", 4))
		i++;
	envp[i] = NULL;	

	/* alles, was dahinter kommt, abschneiden */	
	if (env[0] && env[-1])
	{
		*env++ = 0;			/* kill it */
		while (*env++);
	}
	else
		++env;
	
	/* Parameterstart */
	startpar = env;
	
	while (*env)
	{
		count++;
		while (*env++);
	}
	
	if (count < MIN_ARGV_COUNT)
	{
		myargv = minargv;
	}
	else
	{
		/* Speicher fuer neuen Argument-Vektor */
		myargv = Malloc ((count+1) * sizeof (char *));
		malloced = 1;
	}
	env = startpar;
	
	count = 0;
	while (*env)
	{
		myargv[count++] = env;
		while (*env++);
	}
	myargv[count] = NULL;
	
	/* m”glichst viele Parameter in alte Kommandozeile */	
	{
		int i;
	
		*oldline = 0;
		i = 1;
		
		while ((i < count) && (strlen (oldline) + strlen (myargv[i]) < 120))
		{
			if (i > 1) strcat (oldline, " ");
			strcat (oldline, myargv[i]);
			i++;
		}
		
		if (i < count) strcat (oldline, " ...");
	}

	
	/* und ...argvmain() starten */
	count = argvmain (count, myargv, envp);	
	if (malloced)
		Mfree (myargv);
	return count;
}
