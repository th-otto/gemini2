/*
 * @(#) MParse\IOUtil.h
 * @(#) Stefan Eissing, 06. Februar 1993
 */



#ifndef __ioutil__
#define __ioutil__

BASPAG *getactpd (void);
SYSHDR *GetSysHdr (void);
long GetHz200Timer (void);


int GetStdHandle (int std_stream, int dupHandles, int *opened);
int is_a_tty (int handle);

char *TmpFileName (MGLOBAL *M);

#endif	/* __ioutil__ */