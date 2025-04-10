/*
 * @(#) Mupfel\init.h
 * @(#) Gereon Steffens & Stefan Eissing, 30. Mai 1993
 *
 *  -  definitions for init.c
 */
 
#ifndef M_INIT__
#define M_INIT__

#define MAX_SECTORS		40
#define SECTOR_SIZE		512L


int InitDrive (MGLOBAL *M, const char *command, char drv, 
	short bios_dev, int sides, int sectors, int tracks, char *label, 
	char *trkbuf, int is_locked);

int m_init (MGLOBAL *M, int argc, char **argv);

#endif /* M_INIT__ */