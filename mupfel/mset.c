/*
 * @(#) MParse\mset.c
 * @(#) Stefan Eissing, 21. Februar 1994
 */


#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "ioutil.h"
#include "mset.h"
#include "nameutil.h"
#include "redirect.h"
#include "shellvar.h"
#include "stand.h"


/* internal texts
 */
#define NlsLocalSection "M.mset"
enum NlsLocalText{
OPT_set,	/*[[+-][aefhklnrtuvxCS%\\^]] wert*/
HLP_set,	/*Interne Variablen und Shell-Flags setzen/lîschen*/
MS_ILLOPTION,	/*%s: ungÅltige Option %s\n*/
MS_MISSING,		/*%s: Option o ohne Parameter\n*/
MS_NOCOMMAND, 	/*%s: -c ohne Parameter\n*/
MS_ILL_LONG_OPTION, 	/*%s: -o ohne gÅltigen Optionsnamen\n*/
};

static int setOptChar (MGLOBAL *M, char option, int switch_value, 
	int argc, char **argv, const char *program_name, int *opt_index);


typedef struct
{
	const char *name;
	char option;
} OPTIONTABLE;

static OPTIONTABLE OptionMap[] =
{
	{ "allexport", 'a' },
	{ "errexit", 'e' },
	{ "noclobber", 'C' },
	{ "noglob", 'f' },
	{ "noexec", 'n' },
	{ "nounset", 'u' },
	{ "verbose", 'v' },
	{ "xtrace", 'x' },
	{ "nogem", 'G' },
	{ "slashconv", 'S' },
	{ "escbackslash", '\\' },
	{ "escpercent", '%' },
	{ "eschat", '^' },
};

static int setLongOption (MGLOBAL *M, char *option, int switch_value, 
	int argc, char **argv, const char *program_name, int *opt_index)
{
	size_t i, len;
	
	len = sizeof (OptionMap) / sizeof (OptionMap[0]);
	for (i = 0; i < len; ++i)
	{
		if (!strcmp (OptionMap[i].name, option))
		{
			return setOptChar (M, OptionMap[i].option, 
				switch_value, argc, argv, program_name, opt_index);
		}
	}
	
	eprintf (M, NlsStr(MS_ILL_LONG_OPTION), program_name);
	return FALSE;
}

static int setOptChar (MGLOBAL *M, char option, int switch_value, 
	int argc, char **argv, const char *program_name, int *opt_index)
{
	switch (option)
	{
		case 'a':
			M->shell_flags.export_all = switch_value;
			break;
		case 'c':
			if ((*opt_index + 1) >= argc)
			{
				eprintf (M, NlsStr(MS_NOCOMMAND), program_name);
				return FALSE;
			}
			else
				M->shell_flags.exec_argv = switch_value;
			break;
		case 'e':
			M->shell_flags.break_on_false = switch_value;
			break;
		case 'f':
			M->shell_flags.no_globber = switch_value;
			break;
		case 'h':
			M->shell_flags.hash_function_def = switch_value;
			break;
		case 'i':
			M->shell_flags.interactive = switch_value;
			break;
		case 'k':
			M->shell_flags.keywords_everywhere = switch_value;
			break;
		case 'l':
			M->shell_flags.local_block = switch_value;
			break;
		case 'n':
			if (!M->shell_flags.interactive)
				M->shell_flags.no_execution = switch_value;
			break;
		
		case 'o':
			*opt_index += 1;
			if (*opt_index >= argc)
			{
				eprintf (M, NlsStr(MS_MISSING), program_name);
				return FALSE;
			}
			else
			{
				setLongOption (M, argv[*opt_index], switch_value, 
					argc, argv, program_name, opt_index);
				break;
			}

		case 'r':
			if (!switch_value && M->shell_flags.restricted)
				return -1;
			else
				M->shell_flags.restricted = switch_value;
			break;

		case 't':
			M->shell_flags.only_one_command = switch_value;
			break;

		case 'u':
			M->shell_flags.no_empty_vars = switch_value;
			break;

		case 'v':
			M->shell_flags.verbose = switch_value;
			break;

		case 'x':
			M->shell_flags.print_as_exec = switch_value;
			break;
#if STANDALONE
		case 'G':
			M->shell_flags.dont_use_gem = switch_value;
			break;
#endif
		case 'S':
			M->shell_flags.slash_conv = switch_value;
			break;
		case 'D':
			M->shell_flags.debug_io = switch_value;
			break;
		case 'C':
			M->shell_flags.dont_clobber = switch_value;
			break;

		case '\\':
		case '%':
		case '^':
			if (switch_value)
				M->tty.escape_char = option;
			else
				M->tty.escape_char = '%';
			break;
	}

	return TRUE;
}

static int setOptions (MGLOBAL *M, char **argv, int argc, int startup,
	const char *program_name)
{
	char *set_string = "aefhklnortuvxSDC\\%^";
#if GEMINI
	char *set_on_startup = "sicr";
#else
	char *set_on_startup = "sicrG";
#endif
	int opt_index;
	
	for (opt_index = 1; argc > opt_index; ++opt_index)
	{
		if (*argv[opt_index] == '-')
		{
			char *opt = argv[opt_index];
			
			if (opt[1] == '-')
				return opt_index;
			
			if (opt[1] == '\0')
			{
				M->shell_flags.verbose = M->shell_flags.print_as_exec = 0;
			}
			
			while (*++opt)
			{
				if (!strchr (set_string, *opt)
					&& !strchr (set_on_startup, *opt))
				{
					
					eprintf (M, NlsStr(MS_ILLOPTION), program_name,
						argv[opt_index]);
					return -1;
				}
				
				if (startup || !strchr (set_on_startup, *opt))
				{
					if (!setOptChar (M, *opt, 1, argc, argv,
						program_name, &opt_index))
						return -1;
				}
			}
		}
		else if (*argv[opt_index] == '+')
		{
			char *opt = argv[opt_index];
	
			while (*++opt)
			{
				if ((!startup && strchr (set_on_startup, *opt))
					|| (!strchr (set_string, *opt)
						&& !strchr (set_on_startup, *opt)))
				{
					eprintf (M, NlsStr(MS_ILLOPTION), argv[opt_index]);
					return -1;
				}
	
				if (!setOptChar (M, *opt, 0, argc, argv,
					program_name, &opt_index))
					return -1;
			}
		}
		else
			return opt_index - 1;
	}

	return opt_index - 1;
}

int SetOptionsAndVars (MGLOBAL *M, int argc, char **argv, 
		int *new_argc, char ***new_argv)
{
	int arg_start, arg_count;
	
	arg_start = setOptions (M, argv, argc, TRUE, argv[0]) + 1;
	if (arg_start <= 0)
		return FALSE;
		
	arg_count = argc - arg_start;
	
	*new_argv = &argv[arg_start];
	*new_argc = arg_count;

	if (arg_count == 0
		&& !M->shell_flags.only_one_command
		&& StdinIsatty (M)
		&& StdoutIsatty (M))
	{
		M->shell_flags.interactive = 1;
	}

	return TRUE;
}

int m_set (MGLOBAL *M, int argc, char **argv)
{
	int arg_start, arg_count;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_set), 
			NlsStr(HLP_set));
	
	if (argc > 1)
	{
		arg_start = setOptions (M, argv, argc, FALSE, argv[0]) + 1;
		if (arg_start <= 0)
			return 1;
	
		arg_count = argc - arg_start;
		
		if (arg_count < 1 && strcmp (argv[1], "--"))
			return 0;
			
		if (!InstallShellVars (M, &argv[arg_start], 
			arg_count, TRUE, TRUE))
		{
			return 1;
		}
	}
	else
	{
		PrintAllNames (M);
	}

	return 0;
}

