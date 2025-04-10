/*
 * @(#) MParse\mset.h
 * @(#) Stefan Eissing, 09. Mai 1991
 */


#ifndef __m_set__
#define __m_set__

#ifdef __command__
extern INTERNALINFO MSetInfo;
#endif /* __command__ */

int SetOptionsAndVars (MGLOBAL *M, int argc, char **argv, 
		int *new_argc, char ***new_argv);
		
int m_set (MGLOBAL *M, int argc, char **argv);

#endif /* __m_set__ */