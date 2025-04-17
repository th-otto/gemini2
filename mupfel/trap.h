/*
 * @(#) Mupfel\trap.h
 * @(#) Stefan Eissing, 17. Januar 1992
 *
 *  -  Trap handling
 */

#ifndef __M_TRAP__
#define __M_TRAP__

#define SIGEXIT		0

void InitTrap (MGLOBAL *M);
void ReInitTrap (MGLOBAL *M, MGLOBAL *new);
void ExitTrap (MGLOBAL *M, MGLOBAL *old);

int CheckSignal (MGLOBAL *M);
int SignalTrap (MGLOBAL *M, int signal);

int m_trap (MGLOBAL *M, int argc, char **argv);

#endif /* __M_TRAP__ */
