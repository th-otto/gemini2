/*******************************************************************
 * @(#) exec_job
 * @(#) Stefan Eissing, 29. Januar 1994
 *
 * Reconstructed from library by Thorsten Otto, 2025
 *******************************************************************/

#include "exec_job.h"

#ifndef FALSE
#  define FALSE 0
#  define TRUE  1
#endif

#ifndef SuperToUser
#  define SuperToUser(sp) Super(sp)
#endif

#ifndef MX_READABLE
#define MX_READABLE		0x40			/* any read OK, no write */
#endif
#ifndef MX_PREFSTRAM
#define MX_PREFSTRAM	0x02
#endif

#ifndef _AESversion
/* BUG: that seems to have used a non-standard AES library where contrl array was only 5 ints */
#define _AESversion _GemParBlk.contrl[5]
#endif

#define KOBOLD_JOB 0x2F10          /* Speicherjob starten                      */
#define KOBOLD_JOB_NO_WINDOW 0x2F11/* Dito, ohne Hauptdialog                   */
#define KOBOLD_CLOSE 0x2F16        /* Dient zum expliziten Schliežen des       */
                                   /* KOBOLD, falls Antwortstatus != FINISHED  */


/* static */ char const kobold_id[9] = "KOBOLD_2";
/* static */ char const kobold_prg[] = "KOBOLD_2.PRG";
/* static */ char prg_path[130];
/* static */ char start_path[130];
/* static */ char *job;
/* static */ _WORD prg_id;
/* static */ _WORD sender_id;
/* static */ int start_mode;
/* static */ int close_it;
/* static */ long prg_length;
static DTA dta; /* FIXME: need not be here */



/* static */ void get_path(char *path)
{
	int drv;
	
	drv = Dgetdrv();
	path[1] = ':';
	path[0] = drv + 'A';
	Dgetpath(path + 2, drv + 1);
}


/* static */ void set_path(const char *path)
{
	Dsetdrv((unsigned char)path[0] - 'A');
	Dsetpath(path + 2);
}


/* static */ long search_cookie(long id)
{
	long oldsp;
	long *jar;
	long val;
	
	oldsp = Super(0);
	jar = *((long **)0x5a0);
	/* FIXME: no check for null */
	while (*jar != 0 && *jar != id)
		jar += 2;
	val = *jar == id ? jar[1] : 0;
	SuperToUser((void *)oldsp);
	return val;
}


/* static */ _WORD wait_for_answer(void)
{
	_WORD msg[8];
	
	do
	{
		/* BUG: discards other messages for the application */
		evnt_mesag(msg);
	} while (msg[0] != KOBOLD_ANSWER);
	return msg[3];
}


/* static */ void send_data(_WORD msgid, char *data, _WORD to)
{
	_WORD msg[8];
	
	msg[0] = msgid;
	msg[1] = sender_id;
	msg[2] = 0;
	*((char **)&msg[3]) = data;
	/* FIXME: clear msg[5-7] */
	appl_write(to, (int)sizeof(msg), msg);
}


/* static */ void send_job(char *data)
{
	prg_id = appl_find(kobold_id);
	if (prg_id < 0)
	{
		send_data(KOBOLD_ANSWER, (char *)-1, sender_id);
	} else
	{
		send_data(KOBOLD_JOB_NO_WINDOW, data, prg_id);
	}
}


/* static */ void execute_kobold(char *userjob)
{
	char cmd[130];
	char old_path[130];
	long timeout;
	
	get_path(old_path);
	set_path(prg_path);
	strcpy(cmd, " KOBOLD_JOB_NO_WINDOW ");
	if (start_mode == 0)
	{
		userjob[6] = '8';
		ltoa((long)userjob, strrchr(cmd, '\0'), 10);
		cmd[0] = (char)strlen(cmd + 1);
		Pexec(0, kobold_prg, cmd, NULL);
		prg_id = -1;
		send_data(KOBOLD_ANSWER, (void *)-1, sender_id);
	} else
	{
		close_it = TRUE;
		strcat(cmd, "0 ");
		cmd[0] = (char)strlen(cmd + 1);
		if (start_mode == 1)
		{
			shel_write(1, 1, 100, start_path, cmd);
			timeout = 0;
			do
			{
				prg_id = appl_find(kobold_id);
				if (prg_id < 0)
				{
					evnt_timer(100, 0);
					timeout += 100;
				} else
				{
					timeout = 10000;
				}
			} while (timeout < 4000);
		} else
		{
			prg_id = shel_write(1, 1, 0, start_path, cmd);
			if (prg_id == 0)
				prg_id = -1;
		}
		send_job(userjob);
	}
	set_path(old_path);
}


/* _BOOL */ int init_kobold(int own_aes_id, char *path)
{
	prg_id = appl_find(kobold_id);
	sender_id = own_aes_id;
	start_mode = 0;
	job = NULL;
	close_it = FALSE;
	if (search_cookie(0x4d616758L) != 0) /* 'MagX' */
	{
		start_mode = 1;
	} else if (search_cookie(0x4d694e54L) != 0 && _AESversion >= 0x400) /* 'MiNT' */
	{
		start_mode = 2;
	}
	prg_path[0] = '\0';
	if (prg_id >= 0)
		return TRUE;
	if (path != NULL)
	{
		char *p;
		
		strcpy(prg_path, path);
		/* BUG: should not use strupr */
		strupr(prg_path);
		/* BUG: should be prg_path */
		p = strrchr(path, '\\'); /* FIXME: also handle '/' */
		if (p != NULL && p[1] == '\0')
			p[0] = '\0';
		strcpy(start_path, prg_path);
		strcat(start_path, "\\");
		strcat(start_path, kobold_prg);
		/* BUG: should save old DTA first */
		Fsetdta(&dta);
		if (Fsfirst(start_path, 0) != 0)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}


/* _BOOL */ int perform_kobold_job(char *userjob)
{
	/* FIXME: there are better ways to detect Mxalloc */
	if (search_cookie(0x4d694e54L) >= 93) /* 'MiNT' */
		job = (char *)Mxalloc(strlen(userjob) + 200, MX_READABLE | MX_PREFSTRAM);
	else
		job = (char *)Malloc(strlen(userjob) + 200);
	if (job != NULL)
	{
		strcpy(job, "#53=#29\r\n");
		strcat(job, userjob);
		strcat(job, "\r\n#0 ?;#1 ?;#15");
	} else
	{
		prg_id = -1;
	}
	if (prg_id >= 0)
	{
		send_job(job);
		return TRUE;
	} else
	{
		if (prg_path[0] != '\0')
		{
			execute_kobold(job);
			return TRUE;
		}
	}
	
	return FALSE;
}


void kobold_job_done(int status)
{
	if (job != NULL)
	{
		Mfree(job);
		job = NULL;
	}
	if (prg_id >= 0)
	{
		if (status > 0 || close_it)
		{
			send_data(KOBOLD_CLOSE, NULL, prg_id);
			wait_for_answer();
			prg_id = -1;
		}
	}
}
