/*
 * @(#) Mupfel\label.h
 * @(#) Gereon Steffens & Stefan Eissing, 20. Mai 1993
 *
 *  -  definitions for label.c
 */
 
#ifndef M_LABEL__
#define M_LABEL__

int SetLabel (MGLOBAL *M, char *drive_path, char *newlabel, 
	const char *cmd);

int m_label (MGLOBAL *M, int argc, char **argv);

#endif	/* M_LABEL__ */