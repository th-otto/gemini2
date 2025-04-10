/*
 * @(#) MParse\ShelFork.h
 * @(#) Stefan Eissing, 08. Februar 1994
 */


#ifndef M_ShelFork__
#define M_ShelFork__

#include "partypes.h"

int ShelFork (MGLOBAL *M, MGLOBAL *new, TREEINFO *tree,
	IOINFO *pipe_out, IOINFO *pipe_in, int close_pipes,
	int *wasRealFork);

#endif /* M_ShelFork__ */