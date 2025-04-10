/*
 * @(#) Mupfel\DupTree.h
 * @(#) Stefan Eissing, 25. April 1992
 *
 */

#ifndef M_DupTree__
#define M_DupTree__

#include "partypes.h"


SUBSTLISTINFO *DupSubstListInfo (MGLOBAL *M, SUBSTLISTINFO *list);
WORDINFO *DupWordInfo (MGLOBAL *M, WORDINFO *w);
IOINFO *DupIoInfo (MGLOBAL *M, IOINFO *io);
FORKINFO *DupForkInfo (MGLOBAL *M, FORKINFO *f);
COMMANDINFO *DupCommandInfo (MGLOBAL *M, COMMANDINFO *cmd);
FUNCINFO *DupFuncInfo (MGLOBAL *M, FUNCINFO *func);
IFINFO *DupIfInfo (MGLOBAL *M, IFINFO *i);
WHILEINFO *DupWhileInfo (MGLOBAL *M, WHILEINFO *w);
FORINFO *DupForInfo (MGLOBAL *M, FORINFO *f);
REGINFO *DupRegInfo (MGLOBAL *M, REGINFO *reg);
CASEINFO *DupCaseInfo (MGLOBAL *M, CASEINFO *c);
PARINFO *DupParInfo (MGLOBAL *M, PARINFO *par);
LISTINFO *DupListInfo (MGLOBAL *M, LISTINFO *list);
TREEINFO *DupTreeInfo (MGLOBAL *M, TREEINFO *tree);

#endif /* M_DupTree__ */

