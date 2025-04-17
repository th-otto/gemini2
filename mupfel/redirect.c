/*
 * @(#) MParse\Redirect.c
 * @(#) Stefan Eissing, 26. MÑrz 1994
 *
 * jr 22.4.1995
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "ioutil.h"
#include "mdef.h"
#include "partypes.h"
#include "redirect.h"
#include "stand.h"
#include "subst.h"

/* internal texts
 */
#define NlsLocalSection "M.redirect"
enum NlsLocalText{
REDIR_CANT_OPEN,  /*Konnte `%s' nicht îffnen (%s)\n*/
REDIR_ILL_STREAM, /*Handle %s kann nicht umgeleitet werden\n*/
REDIR_RESTRICTED, /*Dies ist eine eingeschrÑnkte Shell\n*/
REDIR_FAILED,	/*Umlenkung von %d auf %d fehlgeschlagen (%s)\n*/
REDIR_EXISTS,	/*%s existiert bereits\n*/
};

static int removeRedirection (MGLOBAL *M, IOINFO *io)
{
	int stream;

	stream = io->file & IO_UFD;

	if (!io->is_our_tmpfile && io->filename)
	{
		mfree (&M->alloc, io->filename);
		io->filename = NULL;
	}

	/* Ist der Stream, den wir verÑndern, Åberhaupt legal?
	 * Wenn nicht, tun wir so als wÑre alles ok.
	 */
	if ((stream < 0) || (stream >= MAX_STREAM))
		return TRUE;

	if (io->save_handle >= MINHND)
	{
		STREAMINFO *si = &M->redir.stream[stream];
		int h = si->handle;
		long ret;

		si->handle = io->save_handle;
		si->isatty = io->save_tty;

		ret = Fforce (stream, si->handle);
		if (ret != E_OK)
		{
			dprintf ("RÅckgÑngigmachen der Redirection von %d"
				" auf %d fehlgeschlagen (%s)!\n", stream, 
				si->handle, StrError (ret));
			return FALSE;
		}

		if (io->has_been_opened && h >= 0)
		{
			int max_try = 0;

			/* Hîchstens max_try Mal versuchen wir das Handle <h>
			 * zu schlieûen. Wenn ein Fseek schief geht, dann
			 * brechen wir ab.
			 */
			while (max_try < 255 && (Fseek (0L, h, 1) != -37L))
			{
				++max_try;
				Fclose (h);
				if (M->shell_flags.debug_io)
				{
					dprintf ("handle %d closed %d. time\n", h,
						max_try);
				}
			}

			io->has_been_opened = FALSE;
		}
	}

	return TRUE;
}


static int freeAndFalse (MGLOBAL *M, IOINFO *io)
{
	if (io->filename)
	{
		mfree (&M->alloc, io->filename);
		io->filename = NULL;
	}
	return FALSE;
}

static int installRedirection (MGLOBAL *M, IOINFO *io)
{
	int stream;
	int real_file;

	/* Wir setzen den save-handle auf einen illegalen Wert, um
	 * das RÅckgÑngigmachen der Redirection daran zu erkennen.
	 */
	io->save_handle = ILLHND;
	io->has_been_opened = FALSE;

	stream = io->file & IO_UFD;

	/* Ist der Stream, den wir verÑndern, Åberhaupt legal?
	 * Wenn nicht, tun wir so als wÑre alles ok.
	 */
	if (stream < 0 || stream >= MAX_STREAM) return TRUE;

	{
		int no_clobber = M->shell_flags.dont_clobber;
		long fhandle = -1L;

		if (io->file & IO_CLOBBER) no_clobber = FALSE;

		if (io->is_pipe)
		{
			fhandle = io->pipe_handle;
			io->has_been_opened = TRUE;
			real_file = FALSE;
		}
		else if (!io->is_our_tmpfile)
		{
			char *subst_string, *subst_quoted;

			if (io->filename)
			{
				freeAndFalse (M, io);
			}

			if (!SubstWithCommand (M, io->name, &subst_string, 
				&subst_quoted))
			{
				return FALSE;
			}

			mfree (&M->alloc, subst_quoted);

			io->filename = subst_string;

			real_file = (io->file & IO_DOC)
				|| ((!(io->file & IO_MOV))
					&& ((strlen (io->filename) < 4
						|| io->filename[3] != ':')));

			if (real_file)
			{
				io->filename = NormalizePath (&M->alloc, subst_string,
					M->current_drive, M->current_dir);
				mfree (&M->alloc, subst_string);

				if (io->filename == NULL) return FALSE;
			}
		}
		else
			real_file = TRUE;

		if (io->file & IO_DOC)
		{
			fhandle = Fopen (io->filename, 0);
			if (fhandle < 0L)
			{
				eprintf (M, NlsStr(REDIR_CANT_OPEN), io->filename,
					StrError (fhandle));
				return freeAndFalse (M, io);
			}
			io->has_been_opened = TRUE;
		}
		else if (io->file & IO_MOV)
		{
			if (!strcmp ("-", io->filename))
			{
				fhandle = -1L;
				/* stevie: wirklich schlieûen? 
				 * Fclose (fhandle);
				 */
			}
			else if ((fhandle = atoi(io->filename)) >= MAX_STREAM)
			{
				eprintf (M, NlsStr(REDIR_ILL_STREAM), io->filename);
				return freeAndFalse (M, io);
			}
/*			else
				fd = dup(fd);
*/		}
		else if (!((io->file & IO_PUT) || io->is_pipe))
		{
			fhandle = Fopen (io->filename, 0);
			if (fhandle < 0L)
			{
				eprintf (M, NlsStr(REDIR_CANT_OPEN), io->filename,
					StrError (fhandle));
				return freeAndFalse (M, io);
			}
			io->has_been_opened = TRUE;
		}
		else if (M->shell_flags.restricted && !io->is_pipe)
		{
			eprintf (M, NlsStr(REDIR_RESTRICTED));
			return freeAndFalse (M, io);
		}
		else if ((io->file & IO_APP) 
			&& (fhandle = Fopen(io->filename, 1)) >= 0L)
		{
			io->has_been_opened = TRUE;
			Fseek(0L, (int)fhandle, 2);
		}
		else if (!io->is_pipe)
		{
			int broken = FALSE;
			char eof_char = '\032';

			if (no_clobber && access (io->filename, A_READ, &broken))
			{
				eprintf (M, NlsStr(REDIR_EXISTS), io->filename);
				return freeAndFalse (M, io);
			}
			else if (broken)
				fhandle = -1L;
			else
				fhandle = Fcreate (io->filename, 0);

			if ((fhandle < 0)
				|| (real_file && 
					((1 != Fwrite ((int)fhandle, 1L, &eof_char))
					|| (0 != Fseek (0L, (int)fhandle, 0)))))
			{
				eprintf (M, NlsStr(REDIR_CANT_OPEN), io->filename,
					StrError (fhandle));
				return freeAndFalse (M, io);
			}

			io->has_been_opened = TRUE;
		}

		if (fhandle >= 0L)
		{
			long ret;
			STREAMINFO *si = &M->redir.stream[stream];

			if (fhandle >= 0 && fhandle < MAX_STREAM)
			{
				fhandle = M->redir.stream[fhandle].handle;
			}

			ret = Fforce (stream, (int)fhandle);
			if (ret < E_OK)
			{
				eprintf (M, NlsStr(REDIR_FAILED), stream,
					(int)fhandle, StrError (ret));
				return freeAndFalse (M, io);
			}

			/* Handles merken!
			 */
			io->save_handle = si->handle;
			io->save_tty = si->isatty;
			si->handle = (int)fhandle;
			si->isatty = is_a_tty (si->handle);
		}

	}

	if (real_file) DirtyDrives |= DriveBit (io->filename[0]);

	return TRUE;
}

int DoRedirection (MGLOBAL *M, IOINFO *io)
{
	int retcode = TRUE;

	if (!io || M->shell_flags.no_execution) return TRUE;

	retcode = installRedirection (M, io);
	if (retcode)
	{
		retcode = DoRedirection (M, io->next);
		if (!retcode) removeRedirection (M, io);
	}

	return retcode;
}

int UndoRedirection (MGLOBAL *M, IOINFO *io)
{
	int retcode = TRUE;

	if (!io || M->shell_flags.no_execution) return TRUE;

	retcode = UndoRedirection (M, io->next);
	if (retcode) retcode = removeRedirection (M, io);

	return retcode;
}


static void getStreamHandles (MGLOBAL *M)
{
	int dup_handles, i;

	dup_handles = (MiNTVersion || MagiCVersion || GEMDOSVersion >= 0x30);

	for (i = 0; i < MAX_STREAM; ++i)
	{
		int opened;

		M->redir.stream[i].handle = GetStdHandle (i, dup_handles, &opened);
		M->redir.stream[i].isatty = (is_a_tty (i) != 0);
		M->redir.stream[i].was_opened = (opened != 0);
	}
}


int InitRedirection (MGLOBAL *M, int force_to_con)
{
	int i;

	/* Wenn wir in Gemini interaktiv arbeiten, dann holen wir
	 * uns definitiv die normalen Einstellungen fÅr die
	 * StandardkanÑle unter altem Gemdos.
	 */
	if (!M->shell_flags.system_call 
		&& !M->shell_flags.dont_use_gem
		&& force_to_con
		&& GEMDOSVersion < 0x30)
	{
		M->redir.stream[0].handle = -1;
		M->redir.stream[1].handle = -1;
		M->redir.stream[2].handle = -1;
		M->redir.stream[3].handle = -3;

		for (i = 0; i < MAX_STREAM; ++i)
		{
			Fforce (i, M->redir.stream[i].handle);
			M->redir.stream[i].isatty = 1;
			M->redir.stream[i].was_opened = 0;
		}
	}
	else
		getStreamHandles (M);

	return TRUE;
}

int ReInitRedirection (MGLOBAL *M, MGLOBAL *new)
{
	(void)M;
	getStreamHandles (new);
	return TRUE;
}

void SetupStdErr (MGLOBAL *M)
{
	if (M->redir.stream[2].handle != M->redir.stream[1].handle)
	{
		if (M->redir.stream[2].handle > 0)
			Fclose (M->redir.stream[2].handle);

		M->redir.stream[2] = M->redir.stream[1];
		Fforce (2, M->redir.stream[2].handle);
	}

	if (isatty (2)) Fputchar (2, 0, 1);
}

void ExitRedirection (MGLOBAL *M)
{
	int i;

	for (i = 0; i < MAX_STREAM; ++i)
	{
		if (M->redir.stream[i].was_opened
			&& M->redir.stream[i].handle > 0)
		{
			int h = M->redir.stream[i].handle;
			int max_try = 0;

			/* Hîchstens max_try Mal versuchen wir das Handle <h>
			 * zu schlieûen. Wenn ein Fseek schief geht, dann
			 * brechen wir ab.
			 */
			while (max_try < 255 && (Fseek (0L, h, 1) != -37L))
			{
				++max_try;
				Fclose (h);
				if (M->shell_flags.debug_io)
				{
					dprintf ("handle %d closed %d. time\n", h,
						max_try);
				}
			}
		}
	}
}

int OpenPipe (MGLOBAL *M, IOINFO *left, IOINFO *right)
{
	int pipe[2];
	int retcode;

	memset (left, 0, sizeof (IOINFO));
	memset (right, 0, sizeof (IOINFO));

/* jr: xxx: MagiC hat zwar Fpipe, aber kann nicht per Pexec
   nebenlÑufig starten */
	if (MagiCVersion)
		retcode = EINVFN;
	else
/* jr: eoh (end of hack) */
		retcode = Fpipe (pipe);

	if (retcode != EINVFN)
	{
		if (retcode != 0) return FALSE;

		Fcntl (pipe[1], 0L, F_SETFD);
		Fcntl (pipe[0], 0L, F_SETFD);

		left->file = IO_IN_PIPE;
		left->is_pipe = TRUE;
		left->pipe_handle = pipe[1];

		right->file = IO_OUT_PIPE;
		right->is_pipe = TRUE;
		right->pipe_handle = pipe[0];
	}
	else
	{
		left->filename = TmpFileName (M);
		if (!left->filename) return FALSE;

		left->is_our_tmpfile = TRUE;
		left->file = IO_PUT | 1;

		right->filename = StrDup (&M->alloc, left->filename);
		if (!right->filename) return FALSE;

		right->is_our_tmpfile = TRUE;
		right->file = 0;
	}

	return TRUE;
}

void ClosePipe (MGLOBAL *M, IOINFO *left, IOINFO *right)
{
	if (left && left->filename)
		mfree (&M->alloc, left->filename);

	if (right && right->filename)
	{
		Fdelete (right->filename);
		mfree (&M->alloc, right->filename);
	}

	if (left && left->has_been_opened)
	{
		Fclose (left->pipe_handle);
		left->has_been_opened = FALSE;
	}

	if (right && right->has_been_opened)
	{
		Fclose (right->pipe_handle);
		right->has_been_opened = FALSE;
	}
}

int isatty (int handle)
{
	return is_a_tty (handle); /* stevie */
}

int StdoutIsatty (MGLOBAL *M)
{
	return M->redir.stream[1].isatty;
}

int StdinIsatty (MGLOBAL *M)
{
	return M->redir.stream[0].isatty;
}

int StderrIsatty (MGLOBAL *M)
{
	return M->redir.stream[2].isatty;
}
