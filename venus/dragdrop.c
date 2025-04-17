/*
 * @(#) Gemini\dragdrop.c
 * @(#) Stefan Eissing, 12. November 1994
 *
 * jr 22.4.95
 */

#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\genargv.h"
#include "..\common\strerror.h"

#include "vs.h"

#include "window.h"
#include "dragdrop.h"
#include "draglogi.h"
#include "select.h"
#include "vaproto.h"
#include "venuserr.h"

/* internal texts
 */
#define NlsLocalSection "G.dragdrop.c"
enum NlsLocalText{
T_NOPIPE,	/*Pipe fr Drag&Drop konnte nicht angelegt werden.*/
};

#ifndef WF_OWNER
#define WF_OWNER	20
#endif


#define AP_DRAGDROP	63

#define	DD_OK		0
#define DD_NAK		1
#define DD_EXT		2
#define DD_LEN		3
#define DD_TRASH	4
#define DD_PRINTER	5
#define DD_CLIPBOARD	6

/* timeout in milliseconds */
#define DD_TIMEOUT	4000

/* number of bytes of "preferred
 * extensions" sent by receipient during
 * open
 */
#define DD_NUMEXTS	8
#define DD_EXTSIZE	32L

/* max size of a drag&drop item name */
#define DD_NAMEMAX	128

/* max length of a drag&drop header */
#define DD_HDRMAX	(8+DD_NAMEMAX)


typedef struct
{
	int fd;
	
	char pipename[128];
	
	long exts[8];
	
	void *oldpipesig;
} DDInfo;



DDInfo *InitDragDrop (word owner, word to_window, word mx, word my,
	word kstate)
{
	DDInfo *pd;
	long stat;
	long fd_mask;
	char c;

	pd = calloc (1, sizeof (DDInfo));
	if (pd == NULL) return NULL;
	
	strcpy (pd->pipename, "U:\\pipe\\dragdrop.AA");	
	pd->pipename[17] = pd->pipename[18] = 'A';
	
	pd->fd = -1;
	do 
	{
		pd->pipename[18]++;
		if (pd->pipename[18] > 'Z')
		{
			pd->pipename[17]++;
			pd->pipename[18] = 'A';
			if (pd->pipename[17] > 'Z') break;
		}

		/* FA_HIDDEN means "get EOF if nobody has pipe open 
		   for reading" */
		pd->fd = (int)Fcreate (pd->pipename, FA_HIDDEN);
	}
	while (pd->fd == EACCDN);

	if (pd->fd < 0)
	{
		venusErr (NlsStr (T_NOPIPE));
		free (pd);
		return NULL;
	}

	/* construct and send the AES message */
	{
		word msg[8];

		msg[0] = AP_DRAGDROP;
		msg[1] = apid;
		msg[2] = 0;
		msg[3] = to_window;
		msg[4] = mx;
		msg[5] = my;
		msg[6] = kstate;
		msg[7] = (pd->pipename[17] << 8) | pd->pipename[18];
		
		stat = appl_write (owner, 16, msg);
		
		if (stat == 0)
		{
			Fclose (pd->fd);
			free (pd);
			return NULL;
		}
	}

	/* now wait for a response */
	fd_mask = 1L << pd->fd;
	stat = Fselect (DD_TIMEOUT, &fd_mask, 0L, 0L);

	if (!stat || !fd_mask)
	{	/* timeout happened */
		Fclose (pd->fd);
		free (pd);
		return NULL;
	}

	/* read the 1 byte response */
	stat = Fread (pd->fd, 1L, &c);
	
	if (stat != 1 || c != DD_OK)
	{
		Fclose (pd->fd);
		free (pd);
		return NULL;
	}

	/* now read the "preferred extensions" */
	stat = Fread (pd->fd, DD_EXTSIZE, pd->exts);
	if (stat != DD_EXTSIZE)
	{
		Fclose (pd->fd);
		free (pd);
		return NULL;
	}

	pd->oldpipesig = Psignal (SIGPIPE, (void *)SIG_IGN);

	return pd;
}


void ExitDragDrop (DDInfo *pd)
{
	Psignal (SIGPIPE, pd->oldpipesig);
	Fclose (pd->fd);
	free (pd);
}


/*
 * see if the receipient is willing to accept a certain
 * type of data (as indicated by "ext")
 *
 * Input parameters:
 * fd		file descriptor returned from ddcreate()
 * ext		pointer to the 4 byte file type
 * name		pointer to the name of the data
 * size		number of bytes of data that will be sent
 *
 * Output parameters: none
 *
 * Returns:
 * DD_OK	if the receiver will accept the data
 * DD_EXT	if the receiver doesn't like the data type
 * DD_LEN	if the receiver doesn't like the data size
 * DD_NAK	if the receiver aborts
 */

int TryExtension (DDInfo *pd, long ext, char *name, size_t size)
{
	word hdrlen;
	long i;
	char c;

	/* 4 bytes for extension, 4 bytes for size, 1 byte for
	   trailing 0 */
	hdrlen = 9 + (word)strlen (name);
	i = Fwrite (pd->fd, 2L, &hdrlen);

	/* now send the header */
	if (i != 2) return DD_NAK;
		
	i = Fwrite (pd->fd, 4L, &ext);
	i += Fwrite (pd->fd, 4L, &size);
	i += Fwrite (pd->fd, strlen (name) + 1, name);
	if (i != hdrlen) return DD_NAK;

	/* wait for a reply */
	i = Fread (pd->fd, 1L, &c);
	if (i != 1) return DD_NAK;
		
	return c;
}


#define PIPEDIR		"U:\\pipe"

int TryAtariDragAndDrop (word owner, word to_window, 
	ARGVINFO *A, word mx, word my, word kstate)
{
	DDInfo *pd;
	word dummy;
	char *names;
	int retcode = FALSE;
	
	if (!access (PIPEDIR, A_ISDIR, &dummy))
		return FALSE;
	
	if (to_window > 0)
		wind_get (to_window, WF_OWNER, &owner, &dummy, &dummy, &dummy);
	else
		to_window = -1;
	
	if (owner < 0)
		return FALSE;
	
	WindUpdate (END_UPDATE);
	pd = InitDragDrop (owner, to_window, mx, my, kstate);
	if (pd == NULL)
	{
		WindUpdate (BEG_UPDATE);
		return FALSE;
	}
	
	names = Argv2String (pMainAllocInfo, A, TRUE);
	if (names && TryExtension (pd, 'ARGS', "Geminiobjekte", 
		strlen (names) + 1) == DD_OK)
	{
		Fwrite (pd->fd, strlen (names) + 1, names);
		retcode = TRUE;
	}

	free (names);	
	ExitDragDrop (pd);
	WindUpdate (BEG_UPDATE);
	
	return retcode;
}	

static void *oldPipeSig;

void ddclose (int fd)
{
	Psignal (SIGPIPE, oldPipeSig);
	Fclose (fd);
}

static int ddopen (int pipe_id, char *preferext)
{
	long stat;
	int fd;
	char outbuf[DD_EXTSIZE+1];
	char pipename[] = "U:\\PIPE\\DRAGDROP.AA";

	pipename[18] = pipe_id & 0x00ff;
	pipename[17] = (pipe_id & 0xff00) >> 8;

	stat = Fopen (pipename, 2);
	if (stat < 0) return (int)stat;
	fd = (int)stat;
	
	outbuf[0] = DD_OK;
	strncpy (outbuf + 1, preferext, DD_EXTSIZE);

	oldPipeSig = Psignal (SIGPIPE, (void *)SIG_IGN);

	if (Fwrite (fd, (long)DD_EXTSIZE+1, outbuf) != DD_EXTSIZE+1)
	{
		ddclose (fd);
		return -1;
	}

	return fd;
}

/*
 * ddrtry: get the next header from the drag & drop originator
 *
 * Input Parameters:
 * fd:		the pipe handle returned from ddopen()
 *
 * Output Parameters:
 * name:	a pointer to the name of the drag & drop item
 *		(note: this area must be at least DD_NAMEMAX bytes long)
 * whichext:	a pointer to the 4 byte extension
 * size:	a pointer to the size of the data
 *
 * Returns:
 * 0 on success
 * -1 if the originator aborts the transfer
 *
 * Note: it is the caller's responsibility to actually
 * send the DD_OK byte to start the transfer, or to
 * send a DD_NAK, DD_EXT, or DD_LEN reply with ddreply().
 */

static int ddrtry (int fd, char *name, char *whichext, long *size)
{
	word hdrlen;
	long i;
	char buf[80];

	i = Fread (fd, 2L, &hdrlen);
	if (i != 2)
		return -1;

	if (hdrlen < 9)
	{					/* this should never happen */
		return -1;
	}

	i = Fread (fd, 4L, whichext);
	if (i != 4)
		return -1;

	whichext[4] = 0;
	i = Fread (fd, 4L, size);
	if (i != 4)
		return -1;

	hdrlen -= 8;
	if (hdrlen > DD_NAMEMAX)
		i = DD_NAMEMAX;
	else
		i = hdrlen;

	if (Fread (fd, i, name) != i)
		return -1;

	hdrlen -= (int)i;

	/* skip any extra header */
	while (hdrlen > 80)
	{
		Fread (fd, 80L, buf);
		hdrlen -= 80;
	}
	if (hdrlen > 0)
		Fread (fd, (long)hdrlen, buf);

	return 0;
}

/*
 * send a 1 byte reply to the drag & drop originator
 *
 * Input Parameters:
 * fd:		file handle returned from ddopen()
 * ack:		byte to send (e.g. DD_OK)
 *
 * Output Parameters:
 * none
 *
 * Returns: 0 on success, -1 on failure
 * in the latter case the file descriptor is closed
 */

static int ddreply (int fd, int ack)
{
	char c = ack;

	if (Fwrite (fd, 1L, &c) != 1L)
	{
		Fclose (fd);
	}
	return 0;
}

/* modify this as necessary */
static char ourExts[DD_EXTSIZE] = "ARGSPATH.TXT";

int ReceiveDragAndDrop (word sender, word to_window,
	word mx, word my, word kstate, word pipe_id)
{
	int dummy, pd;
	char txtname[DD_NAMEMAX], ext[5];
	char *cmdline;
	long size;
	
	(void)to_window;
	(void)sender;

	if (!access (PIPEDIR, A_ISDIR, &dummy))
		return FALSE;

	pd = ddopen (pipe_id, ourExts);
	if (pd < 0) return 0;

	for(;;)
	{
		int i = ddrtry (pd, txtname, ext, &size);
		if (i < 0)
		{
			ddclose (pd);
			return 1;
		}
		
		if (!strncmp(ext, "ARGS", 4))
		{
			ARGVINFO A;
			
			cmdline = malloc ((size_t)size+1);
			if (!cmdline)
			{
				ddreply (pd, DD_LEN);
				continue;
			}
			ddreply (pd, DD_OK);
			Fread (pd, size, cmdline);
			ddclose (pd);
			cmdline[size] = 0;

			String2Argv (pMainAllocInfo, &A, cmdline, TRUE);
			free (cmdline);
			DropFiles (mx, my, kstate, &A);
			FreeArgv (&A);
			return 0;
		}
		else if (!strncmp(ext, "PATH", 4))
		{
			char *names;
			word owner, type;

			names = WhatIzIt (mx, my, &type, &owner);

			switch (type)
			{
				case VA_OB_TRASHCAN:
				case VA_OB_SHREDDER:
					ddreply (pd, DD_TRASH);
					break;

				case VA_OB_CLIPBOARD:
					ddreply (pd, DD_CLIPBOARD);
					break;
					
				default:
					ddreply (pd, DD_OK);
					if (names)
					{
						Fwrite (pd, strlen(names)+1, names);
						free (names);
					}
					break;
			}
			
			ddclose (pd);
			return 0;
		}
		else if (!strncmp (ext, ".TXT", 4))
		{
			ddreply (pd, DD_OK);
/*			paste_txt (winid, pd, size);
*/			ddclose (pd);
			return 0;
		}

		ddreply(pd, DD_EXT);
	}
}
