/*
 * @(#)Gemini\TimeEvnt.h
 * @(#)Stefan Eissing, 22. Dezember 1992
 *
 * project: venus
 *
 * description: modul for handling timer events
 *
 */


#ifndef G_TIMEEVNT__
#define G_TIMEEVNT__

/* parameter ist ein optionaler Parameter an die Callback-Routine.
 * Gibt die Callback-Routine TRUE zurÅck, so wird dieser Event
 * erneut mit dem ursprÅnglichen Zeitintervall installiert.
 */
int InstallTimerEvent (ulong time, long parameter, 
	int (*call_back) (long p));
	
void RemoveTimerEvent (long parameter, int (*call_back) (long p));

void EnableTimerEvent (uword *evnt_flags, ulong *time);
void RaisedTimerEvent (void);

int InitTimerEvents (void);
void ExitTimerEvents (void);


#endif /* !G_TIMEEVNT__ */