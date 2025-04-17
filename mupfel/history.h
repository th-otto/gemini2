/*
 * @(#) Mupfel\history.h
 * @(#) Stefan Eissing, 05. Juni 1991
 *
 * jr 970117
 */
 

#ifndef __M_HISTORY__
#define __M_HISTORY__

int InitHistory (MGLOBAL *M);
int ReInitHistory (MGLOBAL *M, MGLOBAL *new);
void ExitHistory (MGLOBAL *M);

const char *GetLastHistoryLine (MGLOBAL *M);

int EnterHistory (MGLOBAL *M, const char *line);

void HistoryResetSaveBuffer (MGLOBAL *M, int *save_buffer);

const char *GetHistoryLine (MGLOBAL *M, int upward, int *save_state,
	const char *pattern);

int m_history (MGLOBAL *M, int argc, char **argv);
int m_fc (MGLOBAL *M, int argc, char **argv);

#endif /* __M_HISTORY__ */