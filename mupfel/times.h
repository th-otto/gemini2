/*
 * @(#) Mupfel\times.h
 * @(#) Stefan Eissing, 29. Juni 1991
 *
 * - Abgelaufene Zeit in der Shell ermitteln
 */
 

#ifndef M_TIMES__
#define M_TIMES__

void InitTimes (MGLOBAL *M);

int m_times (MGLOBAL *M, int argc, char **argv);

#endif /* M_TIMES__ */