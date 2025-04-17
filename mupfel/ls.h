/*
 *	@(#)Mupfel/ls.c
 *	@(#)Julian F. Reschke & Stefan Eissing, 30. April 1991
 */


#ifndef __M_LS__
#define __M_LS__

#ifdef __command__
extern INTERNALINFO MLsInfo;
#endif /* __command__ */

/* Fsfirst mit normalisiertem Argument */
/*int normalized_fsfirst (MGLOBAL *M, char *filename, int attr);*/

/* Test auf . oder .. */
/*int is_not_dot_or_dotdot (char *name);*/

/* True fÅr drv:\ */
/*int might_be_dir (MGLOBAL *M, char *filename);*/


int m_ls (MGLOBAL *M, int argc, char **argv);

/* User and group name functions */

void InitUsersGroups (MGLOBAL *M);
void ExitUsersGroups (MGLOBAL *M);
char *getUserName (MGLOBAL *M, int id);
char *getGroupName (MGLOBAL *M, int id);


#endif /* __M_LS__ */