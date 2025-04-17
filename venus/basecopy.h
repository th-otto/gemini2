/*
 * @(#) Gemini\basecopy.h
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: basic functions to copy/move files
 *
 */


#ifndef G_basecopy__
#define G_basecopy__

typedef struct filecopyinfo
{
	struct filecopyinfo *next;
	
	char src_name[MAX_PATH_LEN];	/* voller Name der Quell-Datei */
	char dest_name[MAX_PATH_LEN];	/* voller Name der Ziel-Datei */
	
	long size;						/* L„nge der Datei */
	uword attributes;				/* Dateiattribute */
	DOSTIME d;						/* Datum des Anlegens */
	
	char *buffer;					/* Buffer mit Datei-Inhalt */
	long bytes_read;				/* eingelesene Bytes */
	word in_handle;					/* Handle der offenen Datei */
	word out_handle;				/* falls nicht alles gelesen
									 * wurde.
									 */
	struct
	{
		unsigned name_was_checked : 1;
		unsigned rename_file : 1;
	} flags;
	
} FILECOPYINFO;


typedef struct 
{
	char *buffer;
	long buffer_len;
	long act_index;
	
	FILECOPYINFO *first_file;
	FILECOPYINFO *act_file;
	
	struct
	{
		unsigned is_move : 1;
		unsigned is_old_gemdos : 1;
	} flags;
	
} BASECOPYINFO;


int InitBaseCopy (BASECOPYINFO *B, int is_move);
void ExitBaseCopy (BASECOPYINFO *B, const int delete_moved_files);

void FlushFilesInBuffer (BASECOPYINFO *B);
int BaseCopyFile (BASECOPYINFO *B, const char *src_name, 
	const char *dest_dir, ulong size, uword attributes, 
	uword date, uword time, int do_rename);

#endif	/* !G_basecopy__ */