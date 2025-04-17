/*
 * @(#) MParse\ParsUtil.h
 * @(#) Stefan Eissing, 18. September 1991
 */

#ifndef __parsutil__
#define __parsutil__

void ParsAndExec (MGLOBAL *M);

int ExecScript (MGLOBAL *M, const char *full_name, int argc, 
	const char **argv, WORDINFO *var_list, void *start_parameter, 
	int *broken);

#endif /* __parsutil__ */