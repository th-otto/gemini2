/*
 * @(#) Mupfel\DupTree.c
 * @(#) Stefan Eissing, 28. Mai 1992
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "mglob.h"

#include "maketree.h"
#include "chario.h"
#include "duptree.h"


SUBSTLISTINFO *DupSubstListInfo (MGLOBAL *M, SUBSTLISTINFO *list)
{
	SUBSTLISTINFO *new = NULL;
	
	if (list)
	{
		new = mmalloc (&M->alloc, sizeof (SUBSTLISTINFO));
		if (!new)
			return NULL;
		
		new->next = DupSubstListInfo (M, list->next);
		new->tree = DupTreeInfo (M, list->tree);
		
		if ((list->next && !new->next) || (list->tree && !new->tree))
			return FreeSubstListInfo (M, new);
	}
	
	return new;
}

WORDINFO *DupWordInfo (MGLOBAL *M, WORDINFO *w)
{
	WORDINFO *tmp;
	size_t len;
	
	if (!w)
		return NULL;
		
	tmp = mcalloc (&M->alloc, 1, sizeof (WORDINFO));
	if (!tmp)
		return NULL;
	
	*tmp = *w;
	len = strlen (w->name) + 1;

	tmp->name = mmalloc (&M->alloc, len);
	tmp->quoted = mmalloc (&M->alloc, len);

	if (!tmp->quoted || !tmp->name)
	{
		mfree (&M->alloc, tmp->name);
		mfree (&M->alloc, tmp);
		return NULL;
	}

	memcpy (tmp->name, w->name, len);
	memcpy (tmp->quoted, w->quoted, len);
	
	tmp->list = DupSubstListInfo (M, w->list);
	tmp->next = DupWordInfo (M, w->next);
	
	if ((w->list && !tmp->list) || (w->next && !tmp->next))
		return FreeWordInfo (M, tmp);
		
	return tmp;
}

IOINFO *DupIoInfo (MGLOBAL *M, IOINFO *io)
{
	IOINFO *new = NULL;
	
	if (io)
	{
		new = mmalloc (&M->alloc, sizeof (IOINFO));
		if (!new)
			return NULL;
		
		*new = *io;
		new->next = DupIoInfo (M, io->next);
		if (io->name)
			new->name = DupWordInfo (M, io->name);

		if (io->is_our_tmpfile && io->filename)
		{
			/* Wenn wir eine IOINFO mit <our_tmpfile_name> != NULL
			 * haben, so heižt das, das wir diese Datei angelegt
			 * haben und sie folglich mehrmals benutzen.
			 * Dies passiert (nur?) bei HERE_DOCs.
			 * stevie: Was sollen wir tun? Die Datei darf nur
			 * gel”scht werden, wenn sie _niemand_ mehr braucht!
			 * Soll der erste sie l”schen oder niemand?
			 */
		}
		if (io->filename)
			new->filename = StrDup (&M->alloc, io->filename);
		
		if ((io->next && !new->next)
			|| (io->name && !new->name)
			|| (io->filename && !new->filename))
		{
			return FreeIoInfo (M, new);
		}
	}
	
	return new;
}

FORKINFO *DupForkInfo (MGLOBAL *M, FORKINFO *f)
{
	FORKINFO *new = NULL;
	
	if (f)
	{
		new = mmalloc (&M->alloc, sizeof (FORKINFO));
		if (!new)
			return NULL;
		
		*new = *f;
		new->io = DupIoInfo (M, f->io);
		new->tree = DupTreeInfo (M, f->tree);

		if ((f->io && !new->io) || (f->tree && !new->tree))
			return FreeForkInfo (M, new);		
	}
	
	return new;
}

COMMANDINFO *DupCommandInfo (MGLOBAL *M, COMMANDINFO *cmd)
{
	COMMANDINFO *new = NULL;
	
	if (cmd)
	{
		new = mmalloc (&M->alloc, sizeof (COMMANDINFO));
		if (!new)
			return NULL;
		
		*new = *cmd;
		new->io = DupIoInfo (M, cmd->io);
		new->args = DupWordInfo (M, cmd->args);
		new->set = DupWordInfo (M, cmd->set);

		if ((cmd->io && !new->io) || (cmd->args && !new->args)
			|| (cmd->set && !new->set))
		{
			return FreeCommandInfo (M, new);
		}		
	}
	
	return new;
}

FUNCINFO *DupFuncInfo (MGLOBAL *M, FUNCINFO *func)
{
	FUNCINFO *new = NULL;
	
	if (func)
	{
		new = mmalloc (&M->alloc, sizeof (FUNCINFO));
		if (!new)
			return NULL;
		
		*new = *func;
		new->name = DupWordInfo (M, func->name);
		new->commands = DupTreeInfo (M, func->commands);
		
		if ((func->name && !new->name)
			|| (func->commands && !new->commands))
		{
			return FreeFuncInfo (M, new);
		}
	}
	
	return new;
}

IFINFO *DupIfInfo (MGLOBAL *M, IFINFO *i)
{
	IFINFO *new = NULL;
	
	if (i)
	{
		new = mmalloc (&M->alloc, sizeof (IFINFO));
		if (!new)
			return NULL;
		
		*new = *i;	
		new->condition = DupTreeInfo (M, i->condition);
		new->then_case = DupTreeInfo (M, i->then_case);
		new->else_case = DupTreeInfo (M, i->else_case);
		
		if ((i->condition && !new->condition)
			|| (i->then_case && !new->then_case)
			|| (i->else_case && !new->else_case))
		{
			return FreeIfInfo (M, new);
		}
	}
	
	return new;
}

WHILEINFO *DupWhileInfo (MGLOBAL *M, WHILEINFO *w)
{
	WHILEINFO *new = NULL;
	
	if (w)
	{
		new = mmalloc (&M->alloc, sizeof (WHILEINFO));
		if (!new)
			return NULL;
		
		*new = *w;	
		new->condition = DupTreeInfo (M, w->condition);
		new->commands = DupTreeInfo (M, w->commands);
		
		if ((w->condition && !new->condition)
			|| (w->commands && !new->commands))
		{
			return FreeWhileInfo (M, new);
		}
	}
	
	return new;
}

FORINFO *DupForInfo (MGLOBAL *M, FORINFO *f)
{
	FORINFO *new = NULL;
	
	if (f)
	{
		new = mmalloc (&M->alloc, sizeof (FORINFO));
		if (!new)
			return NULL;
			
		*new = *f;
		new->name = DupWordInfo (M, f->name);
		new->commands = DupTreeInfo (M, f->commands);
		new->list = DupCommandInfo (M, f->list);
		
		if ((f->name && !new->name)
			|| (f->commands && !new->commands)
			|| (f->list && !new->list))
		{
			return FreeForInfo (M, new);
		}
	}
	
	return new;
}

REGINFO *DupRegInfo (MGLOBAL *M, REGINFO *reg)
{
	REGINFO *new = NULL;
	
	if (reg)
	{
		new = mmalloc (&M->alloc, sizeof (REGINFO));
		if (!new)
			return NULL;
		
		*new = *reg;
		new->next = DupRegInfo (M, reg->next);
		new->matches = DupWordInfo (M, reg->matches);
		new->commands = DupTreeInfo (M, reg->commands);
		
		if ((reg->next && !new->next)
			|| (reg->matches && !new->matches)
			|| (reg->commands && !new->commands))
		{
			return FreeRegInfo (M, new);
		}
	}
	
	return new;
}

CASEINFO *DupCaseInfo (MGLOBAL *M, CASEINFO *c)
{
	CASEINFO *new = NULL;
	
	if (c)
	{
		new = mmalloc (&M->alloc, sizeof (CASEINFO));
		if (!new)
			return NULL;
		
		*new = *c;
		new->name = DupWordInfo (M, c->name);
		new->case_list = DupRegInfo (M, c->case_list);
		
		if ((c->name && !new->name) 
			|| (c->case_list && !new->case_list))
		{
			return FreeCaseInfo (M, new);
		}
	}
	
	return new;
}

PARINFO *DupParInfo (MGLOBAL *M, PARINFO *par)
{
	PARINFO *new = NULL;
	
	if (par)
	{
		new = mmalloc (&M->alloc, sizeof (PARINFO));
		if (!new)
			return NULL;
		
		*new = *par;	
		new->tree = DupTreeInfo (M, par->tree);
		
		if (par->tree && !new->tree)
			return FreeParInfo (M, new);
	}
	
	return new;
}

LISTINFO *DupListInfo (MGLOBAL *M, LISTINFO *list)
{
	LISTINFO *new = NULL;
	
	if (list)
	{
		new = mmalloc (&M->alloc, sizeof (LISTINFO));
		if (!new)
			return NULL;
		
		*new = *list;
		new->left = DupTreeInfo (M, list->left);
		new->right = DupTreeInfo (M, list->right);
		
		if ((list->left && !new->left)
			|| (list->right && !new->right))
		{
			return FreeListInfo (M, new);
		}
	}
	
	return new;
}


TREEINFO *DupTreeInfo (MGLOBAL *M, TREEINFO *tree)
{
	void *new = NULL;
	void *p;
	
	if (!tree)
		return NULL;

	p = tree;
	switch (tree->typ & COMMASK)
	{
		case TCOM:
			new = DupCommandInfo (M, p);
			break;
			
		case TPAR:
			new = DupParInfo (M, p);
			break;
			
		case TFIL:
		case TLIST:
		case TAND:
		case TORF:
			new = DupListInfo (M, p);
			break;
			
		case TIF:
			new = DupIfInfo (M, p);
			break;
			
		case TWHILE:
		case TUNTIL:
			new = DupWhileInfo (M, p);
			break;
			
		case TCASE:
			new = DupCaseInfo (M, p);
			break;
			
		case TFORK:
			new = DupForkInfo (M, p);
			break;
			
		case TFOR:
			new = DupForInfo (M, p);
			break;
			
		case TFUNC:
			new = DupFuncInfo (M, p);
			break;
		
		default:
			dprintf ("FreeTreeInfo: unbekannter Typ!\n");
			break;
	}

	return new;
}

