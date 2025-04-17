/*
 * @(#)Gemini\TimeEvnt.c
 * @(#)Stefan Eissing, 20. M„rz 1993
 *
 * project: venus
 *
 * description: modul for handling timer events
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <aes.h>

#include "vs.h"

#include "timeevnt.h"
#include "util.h"
#include "venuserr.h"


typedef struct t_event
{
	struct t_event *next;
	unsigned long requested_time;
	long evnt_time;
	long parameter;
	int (*call_back) (long parameter);
} TimerInfo;

static TimerInfo *TList = NULL;
static unsigned long installed_time;
static char timer_event_pending = FALSE;

static long timeSinceLastInstallation (void)
{
	unsigned long actual_time;
	long elapsed_time;
	
	actual_time = GetHz200 ();
	if (actual_time < installed_time)
	{
		elapsed_time = actual_time + (ULONG_MAX - installed_time);
	}
	else
		elapsed_time = actual_time - installed_time;

	/* Vergangene Zeit in Millisekunden
	 */
	return elapsed_time * 5L;
}
	
/* Installiere Event in der Liste. Die Liste ist zeitlich
 * sortiert in der Reihenfolge, in der die Events auftreten
 * sollen.
 */
static void installTimerInfo (TimerInfo *ti)
{
	TimerInfo **plist;

	ti->evnt_time = ti->requested_time;
	plist = &TList;
	while (*plist != NULL && (*plist)->evnt_time <= ti->evnt_time)
	{
		ti->evnt_time -= (*plist)->evnt_time;
		plist = &(*plist)->next;
	}
	
	ti->next = *plist;
	*plist = ti;
}


int InstallTimerEvent (ulong time, long parameter, 
	int (*call_back) (long p))
{
	TimerInfo *ti;
	
	ti = malloc (sizeof (TimerInfo));
	if (ti == NULL) return FALSE;

	memset (ti, 0, sizeof (TimerInfo));
	ti->requested_time = time;
	ti->parameter = parameter;
	ti->call_back = call_back;
	
	installTimerInfo (ti);
		
	return TRUE;
}


/* Entferne Events, die mit dem entprechenden Parameter und
 * Callback installiert sind.
 */
void RemoveTimerEvent (long parameter, int (*call_back) (long p))
{
	TimerInfo *ti, **plist;
	
	plist = &TList;
	ti = TList;
	while (ti != NULL)
	{
		if (ti->parameter == parameter && ti->call_back == call_back)
		{
			TimerInfo *tmp;
			
			/* Wenn wir den ersten Eintrag in der Liste l”schen,
			 * dann h„ngt dieser nicht mehr und pending muž falsch
			 * werden.
			 */
			if (timer_event_pending)
				timer_event_pending = (ti != TList);

			tmp = ti;
			ti = ti->next;
			*plist = ti;
			free (tmp);
		}
		else
		{
			plist = &ti->next;
			ti = ti->next;
		}
	}
}


void EnableTimerEvent (uword *evnt_flags, ulong *time)
{
	/* Wenn dieses Flag hier TRUE ist, dann hatten wir einen
	 * Timerevent angemeldet, der aber uns noch nicht vom AES
	 * gemeldet wurde. Daher berprfen wir hier, ob er mittler-
	 * weile eingetreten ist, bzw. korrigieren den Timerwert.
	 */
	if (timer_event_pending)
	{
		RaisedTimerEvent ();
	}
	
	/* Trage Werte aus erstem Eintrag in der Liste ein.
	 */
	if (TList != NULL)
	{
		*evnt_flags |= MU_TIMER;
		*time = TList->evnt_time;
		installed_time = GetHz200 ();
		timer_event_pending = TRUE;
	}
	else
	{
		*evnt_flags &= ~MU_TIMER;
		*time = 0L;
		timer_event_pending = FALSE;
	}
}


void RaisedTimerEvent (void)
{
	TimerInfo *ti, *list, *first_pending;
	long elapsed_time;

	timer_event_pending = FALSE;
	elapsed_time = timeSinceLastInstallation ();
	
	/* f„llige Events aus der Liste aush„ngen
	 */
	ti = TList;
	while (ti != NULL && ti->evnt_time <= elapsed_time)
	{
		elapsed_time -= ti->evnt_time;
		ti = ti->next;
	}
	
	if (ti != NULL)
		ti->evnt_time -= elapsed_time;

	list = TList;
	first_pending = TList = ti;
	
	/* Nun sollte TList nur noch Events enthalten, die noch nicht
	 * eingetreten sind und ti zeigt auf den Anfang der List von
	 * Events, die alle jetzt abgearbeitet werden mssen.
	 */
	while (list != first_pending)
	{
		ti = list->next;
		if (list->call_back (list->parameter))
		{
			/* Und bei TRUE erneut installieren
			 */
			installTimerInfo (list);
		}
		else
			free (list);
		list = ti;
	}
}


int InitTimerEvents (void)
{
	return TRUE;
}


void ExitTimerEvents (void)
{
	TimerInfo *ti;
	
	while (TList != NULL)
	{
		ti = TList;
		TList = TList->next;
		
		free (ti);
	}
}