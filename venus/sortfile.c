/*
 * @(#) Gemini\sortfile.c
 * @(#) Stefan Eissing, 25. Februar 1995
 *
 * description: enth„lt Funktionen zum Sortieren von FileInfolisten 
 */

#include <string.h>
#include <aes.h>
#include <ctype.h>
#include <nls\nls.h>

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "filewind.h"
#include "myalloc.h"
#include "venuserr.h"
#define SORT_TYPE	FileInfo
#include "sortfile.h"


/* internal texts
 */
#define NlsLocalSection		"Gmni.sortfile"
enum NlsLocalText{
T_MEMERR,	/*Es ist nicht genug Speicher vorhanden, um alle
 Dateien anzuzeigen!*/
};

static long strNumberCompare (const char *s1, const char *s2)
{
	const char *n = strpbrk (s1, "0123456789");
	size_t i;
	size_t len2;
	
	if (!n)
		return strcmp (s1, s2);
	
	i = n - s1;
	len2 = strlen(s2);
	if (i < len2 && isdigit(s2[i]) && !strncmp (s1, s2, i))
	{
		long n1, n2;
		n1 = atol (s1+i);
		n2 = n1 - atol (s2+i);
		if (!n2)
			return strNumberCompare (s1+i+1, s2+i+1);
		else
			return n2;
	}
	else
		return strcmp (s1, s2);
}

static long cmpnumber (FileInfo *f1, FileInfo *f2)
{
	return (((long)f1->number) - f2->number);
}

static long cmpname (FileInfo *f1, FileInfo *f2)
{
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
	
	return strNumberCompare (f1->name, f2->name);
}

static long cmpsize (FileInfo *f1, FileInfo *f2)
{
	register long l;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;

	l =  (f2->size - f1->size);
	if(l < 0)
		return -1;
	if(l > 0)
		return 1;
	return cmpname(f1,f2);
}

static long cmpdate(FileInfo *f1, FileInfo *f2)
{
	register long l;
	word folderdiff;
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
	
	if(f1->date == f2->date)
		l = ((long)f1->time) - f2->time;
	else
		l = ((long)f1->date) - f2->date;
	if(l < 0)
		return 1;
	if(l > 0)
		return -1;	
	return cmpname(f1,f2);
}

static long cmptype (FileInfo *f1, FileInfo *f2)
{
	word folderdiff, i;
	char *e1, *e2, tmp[2];
	
	folderdiff = (f2->attrib & FA_FOLDER) - (f1->attrib & FA_FOLDER);
	if (folderdiff)
		return folderdiff;
	
	strcpy (tmp, " ");
	e1 = strrchr (f1->name, '.');
	e2 = strrchr (f2->name, '.');
	if (e1 == NULL)
		e1 = tmp;
	if (e2 == NULL)
		e2 = tmp;
		
	if ((i = strcmp (e1, e2)) == 0)
		i = strcmp (f1->name, f2->name);
	
	return i;
}

static void swapPointer(SORT_TYPE **list,word i, word j)
{
	register SORT_TYPE *tmp;
	
	tmp = list[i];
	list[i] = list[j];
	list[j] = tmp;
}

static void swapFileInfo(SORT_TYPE **list,word i, word j)
{
	FileInfo filebuffer, **filelist;
	
	filelist = (FileInfo **)list;
	filebuffer = *(filelist[i]);
	*(filelist[i]) = *(filelist[j]);
	*(filelist[j]) = filebuffer;
}

/* Quick-Sort aus K&R 2.edition S.110
*/
static void (* swapFunction)(SORT_TYPE **list,word i, word j);


static void myqSort (SORT_TYPE **list, long (*cmpfiles)
	(SORT_TYPE *s1, SORT_TYPE *s2),	word left, word right)
{
	register word i,last;
	
	if(left >= right)
		return;
	
	swapFunction (list,left,(right + left)/2);
	last = left;
	for(i = left + 1; i <= right; i++)
	{
		if (cmpfiles(list[i],list[left]) < 0)
			swapFunction (list, ++last, i);
	}
	swapFunction (list, left, last);
	myqSort (list, cmpfiles, left, last - 1);
	myqSort (list, cmpfiles, last + 1, right);
}


static void filesort (FileWindow_t *fwp, long (*cmpfiles)
	(SORT_TYPE *s1, SORT_TYPE *s2))
{
	FileBucket *bucket;
	FileInfo **fplist;				/* fr Array von FileInfopointern */
	word i, nr;
	
	if (fwp->fileanz > 1)
	{
		fplist = malloc (fwp->fileanz * sizeof (FileInfo *));
		if (fplist != NULL)
		{
			word start;
			
			bucket = fwp->files;
			nr = 0;
			if ((bucket) 
				&& (bucket->finfo[0].attrib & FA_FOLDER)
				&& (!strcmp (bucket->finfo[0].name, "..")))
			{
				start = 1;
			}
			else
				start = 0;
			
			while (bucket)
			{
				for(i = 0; i < bucket->usedcount; ++i)
				{
					fplist[nr] = &(bucket->finfo[i]);
					++nr;
				}
				bucket = bucket->nextbucket;
			}
			
			swapFunction = swapFileInfo;
			myqSort (fplist, cmpfiles, start, fwp->fileanz - 1);
			
			free (fplist);
		}
		else
		{
			venusErr (NlsStr (T_MEMERR));
		}
	}
}

void SortFileWindow (FileWindow_t *fwp, SortMode mode)
{
	switch (mode)
	{
		case SortNone:
			filesort (fwp, cmpnumber);
			break;
		case SortDate:
			filesort (fwp, cmpdate);
			break;
		case SortName:
			filesort (fwp, cmpname);
			break;
		case SortType:
			filesort (fwp, cmptype);
			break;
		case SortSize:
			filesort (fwp, cmpsize);
			break;
	}

	fwp->sort = mode;
}

SortMode MenuEntryToSortMode (word menuEntry)
{
	static word map[] = 
	{ UNSORT, SORTDATE, SORTNAME, SORTTYPE, SORTSIZE };
	int i;
	
	for (i = 0; i < DIM(map); ++i)
	{
		if (map[i] == menuEntry)
			return (SortMode)i;
	}
	
	return SortName;
}
