/*
 * @(#) Mupfel\shrink.h
 * @(#) Stefan Eissing, 17. Januar 1993
 *
 */


#ifndef M_SHRINK__
#define M_SHRINK__

void InitShrink (MGLOBAL *M);
void ExitShrink (MGLOBAL *M);

int m_shrink (MGLOBAL *M, int argc, char **argv);

#endif	/* !M_SHRINK__ */