/*
 * @(#) Mupfel\ParsInpt.c
 * @(#) Stefan Eissing, 07. August 1994
 *
 * jr 22.4.95
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h" /* xxx */
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "date.h" /* xxx */
#include "lineedit.h"
#include "loctime.h"
#include "..\common\memfile.h"
#include "names.h"
#include "parsinpt.h"
#include "subst.h"
#include "vt52.h"

static int
show_date (MGLOBAL *M)
{
	char tmp[10];
	dosdate fd;
	int len;
	
	fd.d = Tgetdate ();
	
	len = IDTSprintf (tmp, fd.s.year + 80, fd.s.month, fd.s.day);
	mprintf (M, tmp);

	return len;
}

static int
show_time (MGLOBAL *M)
{
	dostime ft;
	
	ft.t = Tgettime ();
	
	return mprintf (M, "%02d:%02d:%02d", ft.s.hour, ft.s.min, 2 * ft.s.sec);
}


static size_t showPrompt (MGLOBAL *M, const char *prompt)
{
	int i = 0;
	int reverse = 0;
	size_t len = 0;

	Pwait3 (1, NULL);	/* jr: vielleicht woanders besser aufgehoben */

	while (prompt[i])
	{
		if (prompt[i]=='%')
		{
			if (prompt[i+1])
			{
				++i;
				switch (prompt[i])
				{
					case '%':
						rawout (M, '%');
						++len;
						break;

					case 'P':
					case 'p':
						len += mprintf (M, "%c:%s", 
							M->current_drive, M->current_dir);
						break;

					case 'N':
					case 'n':
						crlf (M);
						len = 0;
						break;

					case 'D':
					case 'd':
						len += show_date (M);
						break;

					case 'T':
					case 't':
						len += show_time (M);
						break;

					case 'i':
						reverseon (M);
						++reverse;
						break;

					case 'I':
						reverseoff (M);
						--reverse;
						break;
				}
			}
			else
			{
				len += mprintf (M, "$ ");
				break;
			}
		}
		else
		{
			rawout (M, prompt[i]);
			++len;
		}
		++i;
	}
	if (reverse)
		reverseoff (M);

	return len;
}

static void saveAndResetInfo (MGLOBAL *M, PARSINPUTINFO *save)
{
	memcpy (save, &M->pars_input_info, sizeof (PARSINPUTINFO));
	M->pars_input_info.flags.eof_reached = 0;
	M->pars_input_info.flags.first_line = 1;
	M->pars_input_info.line_count = 0L;
	M->pars_input_info.pushed_line = NULL;
	M->pars_input_info.input_line = NULL;
}

void ParsFromStdin (MGLOBAL *M, PARSINPUTINFO *save)
{
	saveAndResetInfo (M, save);

	M->pars_input_info.input_type = fromStdin;
	M->pars_input_info.mem_file = NULL;
	M->pars_input_info.argc = 0;
	M->pars_input_info.argv = NULL;
}

void ParsFromString (MGLOBAL *M, PARSINPUTINFO *save, 
	const char *line)
{
	int i;
	char **nargv;
	const char *cp;

	saveAndResetInfo (M, save);

	M->pars_input_info.input_type = fromString;

	i = 1;
	cp = line;
	while ((cp = strchr (cp, '\n')) != NULL)
	{
		++i;
		++cp;
	}

	nargv = mmalloc (&M->alloc, i * sizeof(const char *));
	M->pars_input_info.argc = i;
	for (i = 0; i < M->pars_input_info.argc; ++i)
	{
		cp = strchr (line, '\n');
		if (!cp)
		{
			nargv[i] = StrDup (&M->alloc, line);
			break;
		}

		nargv[i] = mcalloc (&M->alloc, 1, cp - line + 2);
		strncpy (nargv[i], line, cp - line + 1);
		line = cp + 1;
	}
	M->pars_input_info.argv = nargv;
}

long ParsFromFile (MGLOBAL *M, PARSINPUTINFO *save, 
	const char *file_name)
{
	MEMFILEINFO *mp;
	char *realname;
	long ret;

	realname = NormalizePath (&M->alloc, file_name, 
		M->current_drive, M->current_dir);

	if (!realname) return ERROR;

	mp = mopen (&M->alloc, realname, &ret);
	mfree (&M->alloc, realname);

	if (mp == NULL) return ret;

	saveAndResetInfo (M, save);

	M->pars_input_info.input_type = fromFile;
	M->pars_input_info.mem_file = mp;

	return E_OK;
}

void ParsFromArgv (MGLOBAL *M, PARSINPUTINFO *save, 
	int argc, const char **argv)
{
	saveAndResetInfo (M, save);

	M->pars_input_info.input_type = fromArgv;
	M->pars_input_info.argc = argc;
	M->pars_input_info.argv = argv;
}


void ResetParsInput (MGLOBAL *M)
{
	clearintr (M);
	M->pars_input_info.flags.first_line = 1;
}

int PushBackInputLine (MGLOBAL *M, const char *line)
{
	M->pars_input_info.pushed_line = StrDup (&M->alloc, line);

	return M->pars_input_info.pushed_line != NULL;
}

const char *GetParsInputLine (MGLOBAL *M, int *broken)
{
	const char *ret_line = NULL;

	if (M->pars_input_info.pushed_line != NULL)
	{
		ret_line = M->pars_input_info.pushed_line;
		M->pars_input_info.pushed_line = NULL;

		return ret_line;
	}

	if (M->pars_input_info.flags.eof_reached)
	{
		return NULL;
	}

	switch (M->pars_input_info.input_type)
	{
		case fromStdin:
		{
			const char *prompt = NULL;

			if (M->pars_input_info.input_line)
			{
				mfree (&M->alloc, M->pars_input_info.input_line);
				M->pars_input_info.input_line = NULL;
			}

			if (M->shell_flags.interactive)
			{
				const char *tmp = NULL;

				if (M->pars_input_info.flags.first_line)
				{
					tmp = GetPS1Value (M);
					M->pars_input_info.flags.first_line = 0;
				}
				else
					tmp = GetPS2Value (M);

				if (tmp)	/* jr: meiner Meinung nach ist tmp immer != NULL */
				{
					prompt = StringVarSubst (M, tmp);
					if (!prompt)
					{
						*broken = TRUE;
						return NULL;
					}
				}
			}

			if (!intr (M))
			{
				M->pars_input_info.input_line = 
					ReadLine (M, TRUE, prompt, showPrompt, broken);
			}

			mfree (&M->alloc, prompt);

			if (M->pars_input_info.input_line == NULL)
			{
				*broken = TRUE;
				clearintr (M);
				return NULL;
			}

			++M->pars_input_info.line_count;

			ret_line = M->pars_input_info.input_line;
			break;
		}

		case fromFile:
			if (checkintr (M) || !mgets (M->pars_input_info.buffer, 
				PARSINFOBUFFLEN-2, M->pars_input_info.mem_file))
			{
				M->pars_input_info.flags.eof_reached = 1;
				resetintr (M);
				return NULL;
			}

			M->pars_input_info.buffer[PARSINFOBUFFLEN-1] = '\0';
			++M->pars_input_info.line_count;

			ret_line = M->pars_input_info.buffer;
			break;

		case fromString:
		case fromArgv:
			if( (M->pars_input_info.argc 
				<=  M->pars_input_info.current)
				|| M->pars_input_info.flags.eof_reached)
			{
				M->pars_input_info.flags.eof_reached = 1;
				return NULL;
			}
			else
			{
				size_t len;
				int i = M->pars_input_info.current;
				const char *cp = M->pars_input_info.input_line;

				if (!cp)
					cp = M->pars_input_info.argv[i];
				len = strlen (cp);

				if (len >= PARSINFOBUFFLEN - 1)
				{
					strncpy (M->pars_input_info.buffer, 
						cp, PARSINFOBUFFLEN - 1);
					M->pars_input_info.buffer[PARSINFOBUFFLEN-1] = '\0';
					M->pars_input_info.input_line = 
						cp + PARSINFOBUFFLEN;
				}
				else
				{
					++M->pars_input_info.line_count;
					M->pars_input_info.input_line = NULL;

					if (M->pars_input_info.line_count > 1)
						strcpy (M->pars_input_info.buffer, " ");
					else
						M->pars_input_info.buffer[0] = '\0';

					strcat (M->pars_input_info.buffer, cp);
					++M->pars_input_info.current;
				}

				ret_line = M->pars_input_info.buffer;
			}
			break;
	}

	if (M->shell_flags.verbose && ret_line != NULL)
	{
		eprintf (M, "%s", ret_line);
	}

	return ret_line;
}

int ParsEOFReached (MGLOBAL *M)
{
	return M->pars_input_info.flags.eof_reached;
}

size_t ParsLineCount (MGLOBAL *M)
{
	return M->pars_input_info.line_count;
}

void ParsRestoreInput (MGLOBAL *M, PARSINPUTINFO *saved)
{
	if (M->pars_input_info.pushed_line != NULL)
	{
		mfree (&M->alloc, M->pars_input_info.pushed_line);
	}

	if (M->pars_input_info.input_type == fromFile)
	{
		mclose (&M->alloc, M->pars_input_info.mem_file);
	}
	else if (M->pars_input_info.input_type == fromStdin
		&& M->pars_input_info.input_line)
	{
		mfree (&M->alloc, M->pars_input_info.input_line);
	}
	else if (M->pars_input_info.input_type == fromString)
	{
		int i;

		for (i = 0; i < M->pars_input_info.argc; ++i)
			mfree (&M->alloc, M->pars_input_info.argv[i]);

		mfree (&M->alloc, M->pars_input_info.argv);
	}

	memcpy (&M->pars_input_info, saved, sizeof (PARSINPUTINFO));
}

