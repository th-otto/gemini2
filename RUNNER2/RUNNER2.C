/*
 * @(#) Runner2\runner2.c
 * @(#) Gereon Steffens & Stefan Eissing, 14. Februar 1993
 *
 * jr 16.5.95
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <aes.h>
#include <vdi.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\toserr.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define DEBUG	0

#define RN_IOERR	"I/O-Fehler beim Zugriff auf `%s'"
#define RN_GENERR	"Fehler beim Zugriff auf `%s'"
#define RN_NOARGS	"[1][RUNNER benîtigt Parameter!][OK]"
#define RN_CANTEXEC "Kann `%s' nicht starten"
#define RN_NOMEM	"Nicht genug freier Speicher fÅr vorhanden"
#define RN_NOPGM	"`%s' ist keine Programmdatei"
#define RN_LOADERR	"I/O-Fehler beim Laden von `%s'"
#define RN_WRONG_FORMAT	"Falsches Format der Parameterdatei"
#define RN_PRESSCR	"Weiter mit <RETURN>"

char *
sccs_id (void)
{
	return "@(#)Runner2, Copyright (c) Stefan Eissing et. al, "__DATE__;
}

typedef struct
{
	char *restart_program;
	COMMAND restart_parameter;
	
	char *start_program;
	COMMAND start_parameter;
	COMMAND pexec_parameter;
	
	char *start_path;
	
	int is_gem;
	int wait_key;
	
	char *exec_env;
	
	char *file_buffer;
} RUNINFO;

int GEMversion;
char *cmddup;

static char nextcmd[300], nextargs[300];
static char newdir[300];


void alert (char *s, ...)
{
	char tmp[300];
	char t[350];
	va_list argpoint;
	
	va_start (argpoint, errmessage);
	vsprintf (tmp, s, argpoint);
	va_end (argpoint);
	
	sprintf (t, "[1][%s][OK]", tmp);
	form_alert (1, t);
}


static char *getnext (char **line, int *broken)
{
	char *cp, *ret;
	
	ret = cp = *line;

	while ((*cp != '\r') && (*cp != '\n'))
	{
		if (*cp == '\0' && cp[1] == '\0')
		{
			/* Dies ist das Ende der Datei
			 */
			*broken = TRUE;
			return ret;
		}
		
		++cp;
	}

	*cp = '\0';
	if (cp[1] == '\n')
		*(++cp) = '\0';

	*line = ++cp;

#if DEBUG
	alert ("nextline: %s", ret);
#endif

	return ret;
}

static char *getNextLine (char **line, int *broken)
{
	while (**line == '#')
	{
		getnext (line, broken);
	}
	
	return getnext (line, broken);
}


int FillRunInfo (RUNINFO *rp, const char *filename)
{
	long len, env_size;
	long fhandle;
	char *line, *tmp;
	int broken = FALSE;
	
	memset (rp, 0, sizeof (RUNINFO));
	fhandle = Fopen (filename, FO_READ);
	if (fhandle < 0) return FALSE;
	
	len = Fseek (0L, (int) fhandle, 2);
	Fseek (0L, (int) fhandle, 0);
	
	rp->file_buffer = calloc (1, len + 5);
	if (rp->file_buffer == NULL)
	{
		Fclose ((int) fhandle);
		alert (RN_NOMEM);
		return FALSE;
	}
	
	Fread ((int) fhandle, len, rp->file_buffer);
	Fclose ((int) fhandle);
	
	line = rp->file_buffer;
	
	rp->restart_program = getNextLine (&line, &broken);

	tmp = getNextLine (&line, &broken);
	strncpy (rp->restart_parameter.command_tail, tmp, 124);
	rp->restart_parameter.command_tail[124] = '\0';
	rp->restart_parameter.length = 
		strlen (rp->restart_parameter.command_tail);
	
	rp->start_program = getNextLine (&line, &broken);
	
	tmp = getNextLine (&line, &broken);
	rp->pexec_parameter.length = (char)atoi (tmp);
	tmp = getNextLine (&line, &broken);
	strncpy (rp->pexec_parameter.command_tail, tmp, 124);
	rp->pexec_parameter.command_tail[124] = '\0';
	if (rp->pexec_parameter.length <= 124)
	{
		rp->start_parameter = rp->pexec_parameter;
	}
	

	rp->start_path = getNextLine (&line, &broken);
	
	tmp = getNextLine (&line, &broken);
	rp->is_gem = tmp[0] != '0';
	rp->wait_key = tmp[1] != '0';
	
	tmp = getNextLine (&line, &broken);
	env_size = atol (tmp);
	if (env_size > 0L)
	{
		rp->exec_env = line;
	}
	
	if (broken)
	{
		free (rp->file_buffer);
		alert (RN_WRONG_FORMAT);
		return FALSE;
	}
	
	return TRUE;
}


void delaccwindows(void)
{
	int window = -1;
	
	while (wind_get (0, WF_TOP, &window) > 0 && window > 0)
	{
		wind_delete (window);
		window = -1;
	}
}

int windnew (void)
{
	if (GEMversion == 0x104 
		|| (GEMversion >= 0x130 && GEMversion != 0x210))
	{
		wind_new ();
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * display desktop-like opening sequence
 */
void gemgimmick (char *title)
{
	static OBJECT dum_ob = {0, -1, -1, G_BUTTON, LASTOB, 0, 0, 0, 0, 0, 0};
	int dummy, hchar;
	
	graf_handle (&dummy, &hchar, &dummy, &dummy);
	
	wind_update (BEG_UPDATE);
	wind_set (0, WF_NEWDESK, 0, 0, 0, 0);
	form_dial (FMD_FINISH, 0, 0, 0, 0, 0, 0, 32767, 32767);

	dum_ob.ob_spec.free_string = title;
	dum_ob.ob_width = 80;
	dum_ob.ob_height = 1;
	rsrc_obfix (&dum_ob, 0);
	dum_ob.ob_height = hchar + 2;

	objc_draw (&dum_ob, 0, 1, 0, 0, 32767, 32767);

	wind_update (END_UPDATE);
}

static void buildcmd (int argc, char **argv, COMMAND *c)
{
	int i;
	
	if (argc > 1)
	{
		strcpy (c->command_tail, argv[1]);
		i = 2;
		while (i < argc 
			&& strlen (c->command_tail) + strlen (argv[i]) + 1 < 125)
		{
			strcat (c->command_tail, " ");
			strcat (c->command_tail, argv[i]);
			++i;
		}
		if (i < argc)
			strcpy (c->command_tail, "");
		c->length = (char)strlen (c->command_tail);
	}
	else
	{
		strcpy (c->command_tail, "");
		c->length = 0;
	}
}

/*
 * gemprg (char *cmd)
 * return TRUE if cmd ends with .PRG, .APP, .ACC or .GTP, FALSE otherwise
 */
static int gemprg (char *cmd)
{
	char *dot;

	dot = strrchr (cmd, '.');
	if (dot++ != NULL)
	{
		return !strcmp (dot, "PRG") 
			|| !strcmp (dot, "APP") 
			|| !strcmp (dot, "ACC")
			|| !strcmp (dot, "GTP");
	}
	else
		return FALSE;
}


/*
 * strfnm (char *path)
 * return pointer to filename part of path
 */
char *strfnm (char *path)
{
	char *s;
	
	if ((s = strrchr (path, '\\')) != NULL)
		return s + 1;
	else
		if (path[1]==':')
			return &path[2];
		else
			return path;
}

/*
 * strrpos (char *str, char ch)
 * return position from right of character ch in str, -1 if not found
 */
int strrpos (const char *str, char ch)
{
	int i;
	
	for (i = (int)strlen (str); i>=0; --i)
		if (str[i] == ch)
			return i;
	return -1;
}

/*
 * strdnm (const char *path)
 * return pointer to allocated copy of directory part of path
 */
char *strdnm (const char *path)
{
	char *new = strdup (path);
	int backslash;

	backslash = strrpos (new, '\\');	/* find last \ */
	if (backslash > -1)
	{
		/* if 1st char or root dir on drive, increment
		 */
		if (backslash==0 || (backslash==2 && new[1]==':'))
			++backslash;
			
		/* terminate string after dir name
		 */
		new[backslash]='\0';
	}
	else
		*new='\0';
		
	return new;
}

static void checkerr (long excode, char *cmdpath, int *broken)
{
	if (excode < 0L)
	{
		*broken = TRUE;

		switch ((int)excode)
		{
			case EFILNF:
				alert ("RUNNER: " RN_CANTEXEC, cmdpath);
				break;
				
			case ENSMEM:
				alert ("RUNNER: " RN_NOMEM);
				break;
				
			case EPLFMT:
				alert ("RUNNER: " RN_NOPGM, cmdpath);
				break;
				
			default:
				if (IsBiosError (excode))
					alert ("RUNNER: " RN_LOADERR, cmdpath);
				else
					alert ("RUNNER: " RN_GENERR, cmdpath);
				break;
		}
	}
}

/*
 * chrapp (char *str, char ch)
 * append ch to str if it's not already the last character
 */
void chrapp (char *str, char ch)
{
	size_t len = strlen (str);
	
	if (len && (str[len - 1] != ch))
	{
		str[len] = ch;
		str[len + 1] = '\0';
	}
}

/* get aes shell args
 */
static void getshellargs (char *nextcmd, char *nextargs, char *newdir)
{
	char *savecmd, *nextdir;

	*nextcmd = *nextargs = '\0';
	shel_read (nextcmd, nextargs);
	nextdir = strdnm (nextcmd);
	
	if (*nextdir=='\0')
	{
		savecmd = strdup (nextcmd);
		strcpy (nextcmd, newdir);

		if (*savecmd != '\\')
			chrapp (nextcmd, '\\');
			
		strcat (nextcmd, savecmd);
		free (savecmd);
	}
	
	free (nextdir);
#if DEBUG
	alert ("Next cmd = %s", nextcmd);
#endif
}

void chdir (char *progname, char *newdir)
{
	char *s;
	
	if (*newdir)
		s = strdup (newdir);
	else
		s = strdnm (progname);

#if DEBUG
	alert ("cd to %s", s);
#endif
	if (s[1] == ':')
		Dsetdrv (toupper(*s)-'A');
		
	Dsetpath (&s[2]);
	strcpy (newdir, s);
	free (s);
}

int argvmain (int argc, char **argv)
{
	RUNINFO R;
	COMMAND cmd, shelcmd;
	long excode;
	int backslash;
	char restartpath[300], dummy[100];
	char cmdpath[300];
	int broken = FALSE;
	
	if (appl_init () < 0) return 1;

	GEMversion = _GemParBlk.global[0];

	if (argc != 2)
	{
		form_alert (1, RN_NOARGS);
		return 1;
	}

	shel_read (restartpath, cmdpath);
	
	if (restartpath[1] != ':')
	{
		strcpy (dummy, restartpath);		/* save name */
		restartpath[0] = Dgetdrv () + 'A';
		restartpath[1] = ':';
		Dgetpath (restartpath + 2, 0);
		chrapp (restartpath, '\\');
		strcat (restartpath, dummy);
	}
	

	if (FillRunInfo (&R, argv[1]))
	{
		int isgem, waitkey;
		
		Fdelete (argv[1]);
		cmd = R.pexec_parameter;
		shelcmd = R.start_parameter;
		strcpy (cmdpath, R.start_program);
		strcpy (newdir, R.start_path);
		isgem = R.is_gem;
		waitkey = R.wait_key;
		
restart:
	
		cmddup = strdup (cmdpath);
		
		if (!windnew ()) delaccwindows ();
	
		if (isgem)
			gemgimmick (strfnm (cmdpath));
		else
		{
			graf_mouse (M_OFF, NULL);
			/* cls, cursor on, normal video */
			Cconws ("\033E\033e\033q"); 
		}

		chdir (cmdpath, newdir);
	
#if DEBUG
		alert ("shel_write(%s, %s)", cmdpath, shelcmd.command_tail);
#endif
	
		shel_write (1, isgem, 1, cmdpath, (char *)&shelcmd);
	
		appl_exit ();
		excode = Pexec (0, cmdpath, &cmd, R.exec_env);
		appl_init ();
		
		/* cursor off */
		Cconws ("\033f");
		if (!isgem) graf_mouse (M_ON, NULL);
		
		checkerr (excode, cmdpath, &broken);

		if (!broken && waitkey)	
		{
			printf ("\n" RN_PRESSCR);
			Crawcin ();
		}
		
		getshellargs (nextcmd, nextargs, newdir);
		
		if (!broken && stricmp (strfnm (nextcmd), strfnm (cmddup)) 
			&& access (nextcmd, A_EXEC, &broken))
		{
			if (!windnew ()) delaccwindows ();
				
			isgem = gemprg (nextcmd);
			waitkey = FALSE;
			gemgimmick (nextcmd);
			strcpy (cmdpath, nextcmd);
			cmd.length = nextargs[0];
			strcpy (cmd.command_tail, &nextargs[1]);
			shelcmd = cmd;
			free (cmddup);
	
#if DEBUG
			alert("Should exec %s", cmdpath);
#endif
	
			goto restart;
		}
	
		/* starte Programm nach */
		if (R.restart_program[1] != ':')
		{
			/* find last \ */
			backslash = strrpos (restartpath, '\\');
			if (backslash > -1)
			{
				/* if 1st char or root dir on drive, increment
				 */
				if (backslash == 0 
					|| (backslash == 2 && restartpath[1] == ':'))
					++backslash;
		
				/* terminate string after dir name
				 */
				restartpath[backslash] = '\0';
			}
			else
				restartpath[0] = '\0';
		
			chrapp (restartpath, '\\');
			strcat (restartpath, R.restart_program);
		}
		else
			strcpy (restartpath, R.restart_program);
	
#if DEBUG
		alert("shel_write(%s)", restartpath);
#endif
	
		shel_write (1, 1, 1, restartpath, 
			(char *)&R.restart_parameter);
	}
	else
		excode = 1;

	appl_exit ();
	
	return (int)excode;
}
