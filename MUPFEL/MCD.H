/*
 * @(#) MParse\mcd.h
 * @(#) Stefan Eissing, 14. Juni 1991
 *
 */


#ifndef __M_CD__
#define __M_CD__

int m_cd (MGLOBAL *M, int argc, char **argv);

int ChangeDir (MGLOBAL *M, const char *path, int store, int *broken);
 
int InitCurrentPath (MGLOBAL *M);
int ReInitCurrentPath (MGLOBAL *old, MGLOBAL *new);
void ExitCurrentPath (MGLOBAL *M);

int ReInitDirStack (MGLOBAL *old, MGLOBAL *new);
void ExitDirStack (MGLOBAL *M);

int m_dirs (MGLOBAL *M, int argc, char **argv);
int m_pushd (MGLOBAL *M, int argc, char **argv);
int m_popd (MGLOBAL *M, int argc, char **argv);

int m_pwd (MGLOBAL *M, int argc, char **argv);

#endif /* __M_CD__ */