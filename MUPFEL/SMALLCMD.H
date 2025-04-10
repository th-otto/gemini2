/*
 * @(#) Mupfel\Smallcmd.h 
 * @(#) Stefan Eissing, 11. Juli 1991
 *
 */

#ifndef __smallcmd__
#define __smallcmd__

int m_true (MGLOBAL *M, int argc, char **argv);
int m_false (MGLOBAL *M, int argc, char **argv);
int m_empty (MGLOBAL *M, int argc, char **argv);
int m_dirname (MGLOBAL *M, int argc, char **argv);
int m_basename (MGLOBAL *M, int argc, char **argv);
int m_getopt (MGLOBAL *M, int argc, char **argv);
int m_dot (MGLOBAL *M, int argc, char **argv);
int m_eval (MGLOBAL *M, int argc, char **argv);
int m_break (MGLOBAL *M, int argc, char **argv);
int m_continue (MGLOBAL *M, int argc, char **argv);
int m_return (MGLOBAL *M, int argc, char **argv);
int m_exit (MGLOBAL *M, int argc, char **argv);
int m_shift (MGLOBAL *M, int argc, char **argv);
int m_echo (MGLOBAL *M, int argc, char **argv);

#endif /* __smallcmd__ */