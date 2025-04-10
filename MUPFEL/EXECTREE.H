/*
 * @(#) MParse\ExecTree.h
 * @(#) Stefan Eissing, 17. MÑrz 1993
 */


#ifndef __exectree__
#define __exectree__

#include "partypes.h"


int ExecTree (MGLOBAL *M, TREEINFO *ptree, int break_on_error,
	IOINFO *pipe_out, IOINFO *pipe_in); 

/* Liste erlaubter Kommando-Typen fÅr <cmd_kind>
 */
#define CMD_FUNCTION	0x01
#define CMD_INTERNAL	0x02
#define CMD_EXTERNAL	0x04
#define CMD_ALL			(CMD_FUNCTION|CMD_INTERNAL|CMD_EXTERNAL)

int ExecArgv (MGLOBAL *M, int argc, const char **argv, 
	int break_on_false, WORDINFO *var_list, int cmd_kind,
	void *start_parameter, int overlay_process, int parallel);

#endif /* __exectree__ */