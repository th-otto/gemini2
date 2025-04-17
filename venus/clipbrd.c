/*
 * @(#)Gemini\clipbrd.c
 * @(#)Stefan Eissing, 14. Februar 1993
 *
 * jr 11.6.95
 */


#include <stddef.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <flydial\clip.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\memfile.h"

#include "vs.h"

#include "clipbrd.h"
#include "fileutil.h"
#include "util.h"
#include "venuserr.h"



/* internal texts
 */
#define NlsLocalSection "G.clipbrd.c"
enum NlsLocalText{
T_NOCLIP,	/*Kann das Verzeichnis des Klemmbrettes nicht 
finden! Ein EinfÅgen ist daher nicht mîglich.*/
T_CREATE,	/*Kann die Datei %s zum EinfÅgen des Textes in 
das Klemmbrett nicht anlegen!*/
T_NOTEXT,	/*Es ist kein Text im Klemmbrett vorhanden.*/
};


/* jr: Text mit Attributen schreiben */

static void attr_write (MEMFILEINFO *mp, char *line, char *attr_line,
	size_t len)
{
	int cur_attr = attr_line[0];
	int i;
	int first_pos = 0;
	char str[2] = "\033 ";
	
	for (i = 0; i < len; i++)
	{
		if (attr_line[i] == cur_attr) continue;

		/* Attribute ausgeben */
		str[1] = 0x80 + cur_attr;
		mwrite (mp, 2, str);
		
		/* Text ausgeben */
		mwrite (mp, i - first_pos, line + first_pos);
		
		cur_attr = attr_line[i];
		first_pos = i;
	}
	
	/* Attribute ausgeben */
	str[1] = 0x80 + cur_attr;
	mwrite (mp, 2, str);

	/* fehlenden Text ausgeben */
	mwrite (mp, i - first_pos, line + first_pos);

	/* Attribute ggfs zurÅcksetzen */
	str[1] = 0x80;
	mwrite (mp, 2, str);
}

int PasteString (char *string, int insert_newline, char *attrib)
{
	char clippath[MAX_PATH_LEN];
	char clippath2[MAX_PATH_LEN];
	MEMFILEINFO *mp, *mp2 = NULL;
	
	if (ClipFindFile ("txt", clippath))
	{
		MFORM form;
		word index;
		
		GrafGetForm (&index, &form);
		GrafMouse (HOURGLASS, NULL);
		
		ClipClear (NULL);

		mp = mcreate (pMainAllocInfo, clippath);
		
		/* jr: spezielle Datei im Wordplus-Format */
		if (attrib && ClipFindFile ("1wp", clippath2))
			mp2 = mcreate (pMainAllocInfo, clippath2);
		
		if (mp != NULL)
		{
			char *line = string;
			char *attr_line = attrib;

			if (insert_newline)
			{
				char *cp;
			
				while ((cp = strchr (line, '\n')) != NULL)
				{
					char *tmp = cp + 1;
					
					do
					{
						*cp-- = '\0';
					}
					while (strlen (line) && (*cp == ' '));
				
					mputs (mp, line);
					
					/* jr: wir fÅhren den Zeiger auf die Attribute
					mit. HÑûlich, aber geht */

					if (mp2)
					{
						attr_line = attrib + (line - string);
						attr_write (mp2, line, attr_line, strlen (line));
						mputs (mp2, "");
					}
					
					line = tmp;
				}
			}
			
			if (*line)
			{
				mwrite (mp, strlen (line), line);
	
				if (mp2)
					attr_write (mp2, line, attr_line, strlen (line));
			}

			mclose (pMainAllocInfo, mp);
			if (mp2) mclose (pMainAllocInfo, mp2);
		}
		else
			venusErr (NlsStr (T_CREATE), clippath);
		
		StripFileName (clippath);
		PathUpdate (clippath);
		
		GrafMouse (index, &form);
	}
	else
		venusErr (NlsStr (T_NOCLIP));
	
	return TRUE;
}


int GetClipText (char *buffer, size_t max_len)
{
	char clippath[MAX_PATH_LEN];
	long fhandle = -1;
	
	if (ClipFindFile ("txt", clippath))
	{
		MFORM form;
		word index;
		
		GrafGetForm (&index, &form);
		GrafMouse (HOURGLASS, NULL);
		
		fhandle = Fopen (clippath, 0);
		if (fhandle >= 0)
		{
			long read;
			
			read = Fread ((int)fhandle, max_len, buffer);
			Fclose ((int)fhandle);
			
			if (read < max_len)
				buffer[read] = '\0';
		}
		
		GrafMouse (index, &form);
	}
	
	if (fhandle < 0)
	{
		venusErr (NlsStr (T_NOTEXT));
		return FALSE;
	}
	else
		return TRUE;
}