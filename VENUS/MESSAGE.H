/*
 * @(#) Gemini\message.h
 * @(#) Stefan Eissing, 06. M„rz 1995
 *
 * description: modul for handling messages
 */

#ifndef G_message__
#define G_message__

#include "..\common\memfile.h"

#ifndef SC_CHANGED
/* 'scrap changed' message */
#define SC_CHANGED  80
/* bitmap representing the scrap file format */
#define SCF_INDEF   0x0000
    /* undefined (may be scrap is empty) */
#define SCF_DBASE   0x0001
    /* data to be loaded into a database (".DBF", ".CSV", ...) */
#define SCF_TEXT    0x0002
    /* text files (".TXT", ".ASC", ".RTF", ".DOC", ...) */
#define SCF_VECTOR  0x0004
    /* vector graphics (".GEM", ".EPS", ".CVG", ".DXF", ...) */
#define SCF_RASTER  0x0008
    /* bitmap graphics (".IMG", ".TIF", ".GIF", ".PCX", ".IFF", ...) */
#define SCF_SHEET   0x0010
    /* spreadsheet data (".DIF", ".WKS", ...) */
#define SCF_SOUND   0x0020
    /* samples, MIDI files, sound, ... (".MOD", ".SND", ...) */
#endif


void MessInit (void);	/* sollten vor bzw. nach appl_init, appl_exit
						 * aufgerufen werden */
void MessExit (void);

void Shutdown (int reason);

int HasShutdown (void);

word HandleMessage (word messbuff[8], word kstate, int *silent);

word StartAcc (WindInfo *wp, word fobj, char *name, 
	ARGVINFO *A, char *command);

word PasteAccWindow (word to_window, word mx, word my, 
	word kstate, ARGVINFO *A);

word WriteAccInfos (MEMFILEINFO *mp, char *buffer);

void ExecAccInfo (const char *line);

void HandlePendingMessages (int with_delay);

void FontChanged (word fileId, word fileSize, 
	word conId, word conSize);

#endif	/* !G_message__ */