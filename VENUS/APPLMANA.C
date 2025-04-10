/*
 * @(#) Gemini\applmana.c
 * @(#) Stefan Eissing, 18. Dezember 1994
 *
 * description: functions to manage start of applications
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "vs.h"

#include "window.h"

#include "applmana.h"
#include "appledit.h"
#include "fileutil.h"
#include "filewind.h"
#include "iconinst.h"
#include "myalloc.h"
#include "select.h"
#include "stand.h"
#include "util.h"
#include "wildcard.h"
#include "venuserr.h"
#include "..\mupfel\mupfel.h"


/* internal texts
 */
#define NlsLocalSection "G.applmana.c"
enum NlsLocalText{
T_MORE1,		/*Was wollen Sie mit `%s' machen?*/
T_MORE2,		/*An[sehen|[Drucken|[Abbruch*/
};


/*
 * make dialog for application installation
 */
void ApplDialog (void)
{
	WindInfo *wp;
	char fname[MAX_FILENAME_LEN], tmp[MAX_PATH_LEN];
	char *applname, *path, *label;
	word index, mode;
	
	applname = path = label = NULL;
	mode = 0;
	
	if (GetOnlySelectedObject (&wp, &index))
	{
		switch (wp->kind)
		{
			case WK_FILE:
			{
				FileWindow_t *fwp = (FileWindow_t *)wp->kind_info;
				FileInfo *pf;
				
				pf = GetSelectedFileInfo (fwp);
				if (!(pf->attrib & FA_FOLDER)
					&& IsExecutable (pf->name))
				{
					applname = pf->name;
					path = fwp->path;
					label = fwp->label;
				}
				break;
			}
			
			case WK_DESK:
			{
				DeskInfo *di = (DeskInfo *)wp->kind_info;
				IconInfo *pii;
				
				pii = GetIconInfo (&di->tree[index]);
				if (pii && (pii->type == DI_PROGRAM)
					&& GetBaseName (pii->fullname, fname, MAX_FILENAME_LEN)
					&& IsExecutable (fname))
				{
					applname = fname;
					label = pii->label;
					strcpy (tmp, pii->fullname);
					StripFileName (tmp);
					path = tmp;
				}
				break;
			}
			
			default:
				break;
		}
	}

	if (applname && !GetStartMode (applname, &mode))
		applname = NULL;
	
	EditApplList (applname, path, label, mode);
}


/* make recursiv Configuration-Infos for applicationrules
 */
int rekursivApplConf (MEMFILEINFO *mp, char *buffer, ApplInfo *pai)
{
	char *wcard;

	if (!pai)
		return TRUE;

	if (!rekursivApplConf (mp, buffer, pai->nextappl))
		return FALSE;
	

	if (strlen (pai->wildcard))
		wcard = pai->wildcard;
	else
		wcard = " ";
			
	sprintf (buffer, "#A@%d@%s@%s@%s@%s", pai->startmode, 
		pai->path, pai->name, wcard, pai->label);
				
	return (mputs (mp, buffer) == 0L);
}


int WriteApplRules (MEMFILEINFO *mp, char *buffer)
{
	return rekursivApplConf (mp, buffer, applList);
}


/*
 * void addApplRule(char *line)
 * scan line for applicationrule and add it to List
 */
void AddApplRule (const char *line)
{
	ApplInfo ai;
	char *cp,*tp;
	
	cp = malloc(strlen(line) + 1);
	if(cp)
	{
		strcpy(cp, line);
		strtok(cp, "@\n");
		if((tp = strtok(NULL, "@\n")) != NULL)
		{
			ai.startmode = atoi(tp);
			if((tp = strtok(NULL, "@\n")) != NULL)
			{
				strcpy(ai.path, tp);
				if((tp = strtok(NULL, "@\n")) != NULL)
				{
					strcpy(ai.name, tp);
					if((tp = strtok(NULL, "@\n")) != NULL)
						strcpy(ai.wildcard, tp);
					if(!tp || !strcmp(ai.wildcard, " "))
						ai.wildcard[0] = '\0';

					if((tp = strtok(NULL, "@\n")) != NULL)
						strcpy(ai.label, tp);
					else
						ai.label[0] = '\0';
			
					InsertApplInfo(&applList, NULL, &ai);
				}
			}
		}

		free(cp);
	}
}

/*
 * Liste der applicationrules freigeben
 */
void FreeApplRules (void)
{
	FreeApplList(&applList);
}


static word askDefaultAppl(const char *file, char *program,
							 char *label, word *mode)
{
	char tmp[1024];
	
	strcpy(label,"");
	*mode = TTP_START;
	
	sprintf(tmp, NlsStr(T_MORE1), file);
	switch (DialAlert(ImSqQuestionMark(), tmp, 0, NlsStr(T_MORE2)))
	{
		case 0:
			strcpy(program,"more");
			break;
		case 1:
			strcpy(program,"print");
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

/*
 * get program responsible for data-file "name"
 * return if an application was found
 */
int GetApplForData (const char *name,char *program, char *label, 
	int ignore_rules, int interactive, word *mode)
{
	ApplInfo *pai;

	*mode = 0;
	pai = applList;
	
	while (pai)
	{
		if (strlen (pai->wildcard) 
			&& FilterFile (pai->wildcard, name, FALSE))
		{
			if (!ignore_rules || !strcmp (pai->wildcard, "*"))
			{
				strcpy (program, pai->path);
				AddFileName (program, pai->name);
				strcpy (label, pai->label);
				*mode = pai->startmode;
				return TRUE;
			}
		}
		
		pai = pai->nextappl;
	}
	
	return interactive? 
		askDefaultAppl (name, program, label, mode) : FALSE;
}

/*
 * word getStartMode(const char *name, word *mode)
 * get startmode for a program
 * return if something was found
 */
word GetStartMode(const char *name, word *mode)
{
	ApplInfo *pai;
	char *cp;
	
	*mode = 0;
	pai = applList;
	while(pai)
	{
		if(!stricmp (name, pai->name))
		{
			*mode = pai->startmode;
			return TRUE;
		}
		else
			pai = pai->nextappl;
	}
	cp = strrchr(name, '.');
	if(cp == NULL)
		return FALSE;
		
	cp++;
	if (IsGemExecutable (name))
	{
		*mode = (GEM_START|WCLOSE_START);
	}
	else if (IsAccessory (name))
		*mode = GEM_START;
	else
		*mode = TOS_START;

	if (TakesParameter (name))
		*mode |= TTP_START;

	if (NewDesk.waitKey && (*mode & (TOS_START|TTP_START)))
		*mode |= WAIT_KEY;
	
	if (NewDesk.ovlStart)
		*mode |= OVL_START;

	return TRUE;
}

void ApRenamed (const char *oldname, const char *newname, int folder)
{
	char tmp[MAX_PATH_LEN], name[MAX_FILENAME_LEN];
	ApplInfo *pai;
	
	if (!folder)
	{
		strcpy (tmp, oldname);
		GetBaseName (tmp, name, MAX_FILENAME_LEN);
		StripFileName (tmp);
	}
	
	pai = applList;
	while (pai)
	{
		if (folder)
		{
			if (IsPathPrefix (pai->path, oldname))
			{
				strcpy (tmp, &pai->path[strlen (oldname)]);
				strcpy (pai->path, newname);
				strcat (pai->path, tmp);
			}
		}
		else if (!stricmp (name, pai->name) && !stricmp (tmp, pai->path))
		{
			strcpy (pai->path, newname);
			GetBaseName (pai->path, pai->name, MAX_FILENAME_LEN);
			StripFileName (pai->path);
			return;
		}
		
		pai = pai->nextappl;
	}
}

void ApNewLabel(char drive, const char *oldlabel, const char *newlabel)
{
	ApplInfo *pai;
	
	pai = applList;
	while(pai)
	{
		if ((drive == pai->path[0])
			&& (!strcmp(oldlabel, pai->label)))
		{
			strcpy(pai->label, newlabel);
		}
		pai = pai->nextappl;
	}
}

word RemoveApplInfo (const char *name)
{
	return DeleteApplInfo (&applList, name);
}
