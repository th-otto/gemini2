/*
 * @(#) MParse\hash.h
 * @(#) Stefan Eissing, 21. Juni 1991
 *
 * Hashing fÅr externe Kommandos
 */


#ifndef __M_HASH__
#define __M_HASH__

typedef struct hashinfo
{
	struct hashinfo *next;
	
	char *command;				/* Der Name des Kommandos */
	char *full_name;			/* Kompletter Pfad zum Kommando */
	char *executer;				/* fÅr Alien Scripts */
		
	int location;				/* Typ des Kommandos von Locate */
	int hits;					/* Treffer */
	int costs;					/* Suchkosten */
	
	struct
	{
		unsigned is_absolute : 1;	/* absolute Pfad */
		unsigned dir_changed : 1;	/* aktuelles dir geÑndert */
	} flags;
		
} HASHINFO;

void InitHash (MGLOBAL *M);
int ReInitHash (MGLOBAL *M, MGLOBAL *new);
void ExitHash (MGLOBAL *M);

void ClearHash (MGLOBAL *M);
void NoticeHash (MGLOBAL *M);


const HASHINFO *GetHashedCommand (MGLOBAL *M, const char *command,
	int increment);
int EnterHash (MGLOBAL *M, const char *command, const char *full_name,
	const char *executer, int location, int costs, int hits);
int RemoveHash (MGLOBAL *M, const char *command);

int m_hash (MGLOBAL *M, int argc, char **argv);

#endif /* __M_HASH__ */
