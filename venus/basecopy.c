/*
 * @(#) Gemini\basecopy.c
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: basic functions to copy/move files
 *
 * jr 22.4.95
 */


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <aes.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "vs.h"

#include "basecopy.h"
#include "cpintern.h"
#include "fileutil.h"
#include "util.h"
#include "venuserr.h"
#include "..\mupfel\mupfel.h"

/* internal texts
 */
#define NlsLocalSection "G.basecopy"
enum NlsLocalText{
T_IOERROR,		/*Fehler %d beim Zugriff auf %s!*/
T_NOFILE,		/*%s konnte nicht gefunden werden. MerkwÅrdig!*/
T_PROTECTED,	/*%s ist schreibgeschÅtzt und konnte daher nicht
 gelîscht werden.*/
T_CANTCREATE,	/*Konnte %s nicht anlegen!*/
T_CANTOPEN,		/*Konnte %s nicht îffnen!*/
T_FULLDISK,		/*Das Ziellaufwerk ist voll!*/
T_WRITEERR,		/*Fehler beim Schreiben in %s!*/
T_READERR,		/*Fehler beim Lesen von %s!*/
T_ILLNAME,		/*Dateiname in %s nicht ermittelbar!*/
T_READFILE,		/*Lese Datei:*/
T_WRITEFILE,	/*Schreibe Datei:*/
T_DELFILE,		/*Lîsche Datei:*/
T_MOVEFILE,		/*Verschiebe Datei:*/
};

#define MIN_KEEPFREE	32767L


static int checkDiskFull (char drive)
{
	DISKINFO di;
	
	if (!Dfree (&di, toupper (drive) - 'A' + 1))
	{
		return (di.b_free == 0L);
	}
	
	return FALSE;
}


int InitBaseCopy (BASECOPYINFO *B, int is_move)
{
	long keep_free;
	long min_size = 8192L + sizeof (FILECOPYINFO);
	char *cp;

	memset (B, 0, sizeof (BASECOPYINFO));
	
	cp = GetEnv (MupfelInfo, "KEEPFREE");
	if (!cp || ((keep_free = atol (cp)) < MIN_KEEPFREE))
		keep_free = MIN_KEEPFREE;

	B->buffer_len = (long)Malloc (-1L);
	B->buffer_len -= keep_free;
	
	if (B->buffer_len < min_size)
		B->buffer_len = min_size;

	while ((B->buffer = Malloc (B->buffer_len)) == NULL)
	{
		B->buffer_len /= 2;
		if (B->buffer_len < min_size)
		{
			return FALSE;
		}
	}
	
	B->flags.is_move = (is_move != 0);
	B->flags.is_old_gemdos = GEMDOSVersion < 0x15;
	
	return TRUE;
}

void ExitBaseCopy (BASECOPYINFO *B, const int delete_moved_files)
{
	FILECOPYINFO *F = B->first_file;
	
	while (F)
	{
		if (F->in_handle == 0)
			Fclose (F->in_handle);
		if (F->out_handle == 0)
			Fclose (F->out_handle);

		/* Lîsche die alte Datei, falls sie kopiert wurde
		 */
		if (delete_moved_files && F->dest_name[0] == '\0')
		{
			int retcode;
			
			DisplayFileName (NlsStr (T_DELFILE), F->src_name);
			retcode = Fdelete (F->src_name);
				
			switch (retcode)
			{
				case EPTHNF:
				case EFILNF:
					venusErr (NlsStr (T_NOFILE), F->src_name);
					break;
		
				case EACCDN:
				case WRITE_PROTECT:
					venusErr (NlsStr (T_PROTECTED), F->src_name);
					break;
		
				default:
					if (retcode)
					{
						venusErr (NlsStr (T_IOERROR), retcode, 
							F->src_name);
					}
					else
					{
						/* Ist gutgegangen
						 */
					}
					break;
			}
		}

		F = F->next;
	}
	
	if (B->buffer)
		Mfree (B->buffer);
}


static int canKeepMoreFiles (BASECOPYINFO *B)
{
	size_t free_space;

	/* freien Platz berechnen
	 */	
	free_space = B->buffer_len - B->act_index;

	/* Setze auf long-Alignment
	 */
	free_space = (free_space / sizeof (long)) * sizeof (long);

	/* Um sicher zu sein, nehmen wir ein biûchen mehr Platz
	 */
	return free_space > (1032L + sizeof (FILECOPYINFO));
}


static int readInBuffer (FILECOPYINFO *F, long read_size)
{
	DisplayFileName (NlsStr (T_READFILE), F->src_name);

	F->bytes_read = Fread (F->in_handle, read_size, F->buffer);
			
	return (F->bytes_read == read_size);
}

static void baseCopyFile (FILECOPYINFO *F, int is_old_gemdos,
	int is_move)
{
	long file_len;

	if (!F->flags.name_was_checked
		 && !CheckValidTargetFile (F->src_name, F->dest_name,
		 	F->flags.rename_file))
	{
		FileWasCopied (F->size);
		return;
	}

	/* stevie: check valid file name
	 */
	F->out_handle = (int)Fcreate (F->dest_name, 0);
	if (F->out_handle <= 0)
	{
		/* Anlegen schiefgegangen
		 */
		venusErr (NlsStr (T_CANTCREATE), F->dest_name);

		AskForAbort (TRUE, F->src_name);
		FileWasCopied (F->size);
		return;
	}
	
	file_len = F->size;
	
	while (F->bytes_read > 0)
	{
		long bytes_written;
		
		DisplayFileName (NlsStr (T_WRITEFILE), F->dest_name);
		bytes_written = Fwrite (F->out_handle, F->bytes_read,
			F->buffer);
		
		if (bytes_written != F->bytes_read)
		{
			if (checkDiskFull (F->dest_name[0]))
				venusErr (NlsStr (T_FULLDISK));
			else
				venusErr (NlsStr (T_WRITEERR), F->dest_name);
			
			Fclose (F->out_handle);
			F->out_handle = 0;
			Fdelete (F->dest_name);

			if (F->in_handle)
			{
				Fclose (F->in_handle);
				F->in_handle = 0;
			}
			
			AskForAbort (TRUE, F->src_name);
			FileWasCopied (F->size);
			return;
		}
		
		file_len -= F->bytes_read;
		if (F->bytes_read > file_len)
			F->bytes_read = file_len;
		
		if ((F->bytes_read > 0)&& !readInBuffer (F, F->bytes_read))
		{
			venusErr (NlsStr (T_READERR), F->src_name);
			
			Fclose (F->out_handle);
			F->out_handle = 0;
			Fdelete (F->dest_name);

			if (F->in_handle)
			{
				Fclose (F->in_handle);
				F->in_handle = 0;
			}

			AskForAbort (TRUE, F->src_name);
			FileWasCopied (F->size);
			return;
		}
	}
	
	/* OK, wir haben die Datei ganz kopiert.
	 */
	Fdatime (&F->d, F->out_handle, 1);
	Fclose (F->out_handle);
	F->out_handle = 0;

	if (F->attributes & ~FA_ARCHIV)
	{
		/* Attribute setzen
		 */
		Fattrib (F->dest_name, 1, F->attributes | FA_ARCHIV);
	}

	if (F->in_handle)
	{
		Fclose (F->in_handle);
		F->in_handle = 0;
	}
	
	/* GRR, wie ich es hasse! Auf GEMDOS kleiner 0x15 klappt das
	 * Setzen des Datums einer gerade erzeugten Datei nicht.
	 */
	if (is_old_gemdos)
	{
		int handle = (int)Fopen (F->dest_name, 2);
		
		if (handle > 0)
		{
			Fdatime (&F->d, handle, 1);
			Fclose (handle);
		}
	}
	
	if (is_move)
		FileWasMoved (F->src_name, F->dest_name);

	F->dest_name[0] = '\0';
	FileWasCopied (F->size);
}


void FlushFilesInBuffer (BASECOPYINFO *B)
{
	FILECOPYINFO *F = B->first_file;
	
	while (F)
	{
		CheckUserBreak ();

		/* Kopiere die Datei
		 */
		baseCopyFile (F, B->flags.is_old_gemdos, B->flags.is_move);
		
		if (F->in_handle)
		{
			Fclose (F->in_handle);
			F->in_handle = 0;
		}
		if (F->out_handle)
		{
			Fclose (F->out_handle);
			F->out_handle = 0;
		}

		F = F->next;
	}
	
	if (B->flags.is_move)
	{
		F = B->first_file;
	
		while (F)
		{
			/* Lîsche die alte Datei, falls sie kopiert wurde
			 */
			if (F->dest_name[0] == '\0')
			{
				int retcode;
				
				DisplayFileName (NlsStr (T_DELFILE), F->src_name);
				retcode = Fdelete (F->src_name);
				
				switch (retcode)
				{
					case EPTHNF:
					case EFILNF:
						venusErr (NlsStr (T_NOFILE), F->src_name);
						break;
			
					case EACCDN:
					case WRITE_PROTECT:
						venusErr (NlsStr (T_PROTECTED), F->src_name);
						break;
			
					default:
						if (retcode)
						{
							venusErr (NlsStr (T_IOERROR), retcode, 
								F->src_name);
						}
						else
						{
							/* Ist gutgegangen
							 */
						}
						break;
				}
			}
		
			F = F->next;
		}
	}
	
	B->act_index = 0L;
	B->first_file = B->act_file = NULL;
	
	return;
}

int BaseCopyFile (BASECOPYINFO *B, const char *src_name, 
	const char *dest_dir, ulong size, uword attributes, uword date, 
	uword time, int do_rename)
{
	size_t next_aligned, read_size;
	FILECOPYINFO *F;
	
	CheckUserBreak ();

	if (!canKeepMoreFiles (B))
	{
		FlushFilesInBuffer (B);
	}
	
	/* Reserviere Platz fÅr die nÑchste FILECOPYINFO-Struktur
	 */
	next_aligned = ((B->act_index + sizeof (long)) 
						/ sizeof (long)) * sizeof (long);
	
	F = (FILECOPYINFO *)&B->buffer[next_aligned];
	B->act_index = next_aligned + sizeof (FILECOPYINFO);
	
	memset (F, 0, sizeof (FILECOPYINFO));

	{
		char *cp;
		
		strcpy (F->dest_name, dest_dir);
		cp = strrchr (src_name, '\\');
		if (cp == NULL)
		{
			/* Kann Namen nicht herausfinden
			 */
			venusErr (NlsStr (T_ILLNAME), src_name);
			AskForAbort (TRUE, src_name);
			return FALSE;
		}

		AddFileName (F->dest_name, ++cp);
		strcpy (F->src_name, src_name);
	}
	
	F->attributes = attributes;
	F->size = size;
	F->d.date = date;
	F->d.time = time;
	F->flags.rename_file = (do_rename != 0);
	
	/* Beim Verschieben auf demselben Laufwerk versuchen wir einen
	 * Rename
	 */
	if (B->flags.is_move && !strncmp (F->src_name, F->dest_name, 3))
	{
		if (!CheckValidTargetFile (F->src_name, F->dest_name,
			F->flags.rename_file))
		{
			FileWasCopied (F->size);
			return TRUE;
		}
		else
			F->flags.name_was_checked = 1;

		if (!Frename (0, F->src_name, F->dest_name))
		{
			DisplayFileName (NlsStr (T_MOVEFILE), F->src_name);
			FileWasCopied (F->size);
			FileWasMoved (F->src_name, F->dest_name);
			return TRUE;
		}
	}
	
	read_size = B->buffer_len - B->act_index;

	if (read_size >= F->size)
		read_size = F->size;
	else
	{
		/* Die Datei ist lÑnger als der Buffer. Wir versuchen erst
		 * einmal vorhandene Dateien wegzuschreiben, falls welche 
		 * da sind.
		 */
		if (B->first_file != NULL)
		{
			FlushFilesInBuffer (B);
			
			return BaseCopyFile (B, src_name, dest_dir, 
					size, attributes, date, time, do_rename);
		}
		
		/* Wir haben schon den ganzen Buffer. Da hilft nix, wir
		 * mÅssen in mehreren Schritten kopieren.
		 */
	}
		
	if ((F->in_handle = (int)Fopen (F->src_name, 0)) < 0)
	{
		/* Kann Datei nicht îffnen
		 */
		venusErr (NlsStr (T_CANTOPEN), F->src_name);
		AskForAbort (TRUE, F->src_name);
		return FALSE;
	}
	
	F->buffer = &B->buffer[B->act_index];

	if (!readInBuffer (F, read_size))
	{
		Fclose (F->in_handle);
		F->in_handle = 0;

		venusErr (NlsStr (T_READERR), src_name);
		AskForAbort (TRUE, src_name);
		return FALSE;
	}
	
	/* So, hier ist soweit alles gutgegangen
	 */
	B->act_index += read_size;
	F->next = NULL;
	
	if (B->first_file == NULL)
	{
		B->act_file = B->first_file = F;
	}
	else
	{
		B->act_file->next = F;
		B->act_file = F;
	}

	if (read_size == F->size)
	{
		/* Wir haben die ganze Datei in den Buffer gelesen.
		 */
		Fclose (F->in_handle);
		F->in_handle = 0;
		return TRUE;
	}
	
	/* Hierhin gelangen wir nur, wenn
	 * a) dies die erste Datei im Buffer ist und
	 * b) diese nicht ganz in den Buffer paût.
	 */
	FlushFilesInBuffer (B);
	return TRUE;
}	