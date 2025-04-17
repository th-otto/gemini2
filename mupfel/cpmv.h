/*
 * @(#) Mupfel\cpmv.h 
 * @(#) Stefan Eissing, 10. Mai 1993
 *
 * jr 30.12.1996
 */

#ifndef __M_CPMV__
#define __M_CPMV__

#define H_DEBUG	0
#define NUMHANDLES	30

typedef struct copyinfo
{
	MGLOBAL *M;
	
	const char *cmd;
	
	struct
	{
		unsigned char is_move;
		
		unsigned char preserve;
		unsigned char recursive;
		unsigned char verbose;
		unsigned char interactive;
		unsigned char archiv;
		unsigned char secure;
		unsigned char non_exist;
		unsigned char force;
		unsigned char confirm;
	}flags;

	int (* copyfile)(struct copyinfo *, char *, char *, char *, char *, int *);

	char *buffer;
	long buffer_len;
	
	int broken;	
} COPYINFO;


void FreeCopyInfoContents (COPYINFO *C);

int cp (COPYINFO *C, char *origfrom, char *origto, char *realfrom, 
	char *realto, int *really_was_copied);
int mv (COPYINFO *C, char *origfrom, char *origto, 
	char *realfrom, char *realto, int *really_was_moved);

int m_cp (MGLOBAL *M, int argc, char **argv);

#endif /* __M_CPMV__ */