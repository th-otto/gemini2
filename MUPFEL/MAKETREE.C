/*
 * @(#) MParse\MakeTree.c
 * @(#) Stefan Eissing, 25. April 1992
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "maketree.h"
#include "chario.h"



TREEINFO *MakeForkInfo (MGLOBAL *M, int flags, TREEINFO *tree)
{
	FORKINFO *ftree;
	
	ftree = mcalloc (&M->alloc, 1, sizeof (FORKINFO));
	if (!ftree)
		return NULL;
	
	ftree->typ = flags | TFORK;
	ftree->tree = tree;
	
	return (TREEINFO *)ftree;
}

TREEINFO *MakeListInfo (MGLOBAL *M, int typ, TREEINFO *left, 
	TREEINFO *right)
{
	LISTINFO *list;
	
	list = mcalloc (&M->alloc, 1, sizeof (LISTINFO));
	if (!list)
		return NULL;
	
	list->typ = typ;
	list->left = left;
	list->right = right;
	
	return (TREEINFO *)list;
}


int DupTokenName (MGLOBAL *M, WORDINFO *token, char **name, 
	char **quoted)
{
	size_t len;
	
	len = strlen (token->name) + 1;

	*name = mmalloc (&M->alloc, len);
	*quoted = mmalloc (&M->alloc, len);
	if (!*name)
	{
		*quoted =NULL;
		return FALSE;
	}
	if (!*quoted || !*name)
	{
		mfree (&M->alloc, *name);
		mfree (&M->alloc, *quoted);
		*name = *quoted =NULL;
		return FALSE;
	}

	memcpy (*name, token->name, len);
	memcpy (*quoted, token->quoted, len);
	
	return TRUE;
}


void *FreeWordInfo (MGLOBAL *M, WORDINFO *w)
{
	if (w)
	{
		FreeWordInfo (M, w->next);
		mfree (&M->alloc, w->name);
		mfree (&M->alloc, w->quoted);
		FreeSubstListInfo (M, w->list);
		
		mfree (&M->alloc, w);
	}
	
	return NULL;
}

void *FreeIoInfo (MGLOBAL *M, IOINFO *io)
{
	if (io)
	{
		FreeIoInfo (M, io->next);
		FreeWordInfo (M, io->name);

		if (io->is_our_tmpfile && io->filename)
		{
			/* Wenn wir eine IOINFO mit <our_tmpfile_name> != NULL
			 * haben, so heižt das, das wir diese Datei angelegt
			 * haben und folglich auch wieder l”schen mssen.
			 * Dies passiert (nur?) bei HERE_DOCs.
			 */
			if (Fdelete (io->filename))
			{
				eprintf (M, "Fehler beim l”schen von %s!\n",
					io->filename);
			}
		}
		mfree (&M->alloc, io->filename);
		mfree (&M->alloc, io);
	}
	return NULL;
}

void *FreeForkInfo (MGLOBAL *M, FORKINFO *f)
{
	if (f)
	{
		FreeIoInfo (M, f->io);
		FreeTreeInfo (M, f->tree);
		
		mfree (&M->alloc, f);
	}
	
	return NULL;
}

void *FreeCommandInfo (MGLOBAL *M, COMMANDINFO *cmd)
{
	if (cmd)
	{
		FreeIoInfo (M, cmd->io);
		FreeWordInfo (M, cmd->args);
		FreeWordInfo (M, cmd->set);
		
		mfree (&M->alloc, cmd);
	}
	
	return NULL;
}


void *FreeFuncInfo (MGLOBAL *M, FUNCINFO *func)
{
	if (func)
	{
		FreeWordInfo (M, func->name);
		FreeTreeInfo (M, func->commands);
		mfree (&M->alloc, func);
	}
	
	return NULL;
}

void *FreeIfInfo (MGLOBAL *M, IFINFO *i)
{
	if (i)
	{
		FreeTreeInfo (M, i->condition);
		FreeTreeInfo (M, i->then_case);
		FreeTreeInfo (M, i->else_case);
		
		mfree (&M->alloc, i);
	}
	
	return NULL;
}

void *FreeWhileInfo (MGLOBAL *M, WHILEINFO *w)
{
	if (w)
	{
		FreeTreeInfo (M, w->condition);
		FreeTreeInfo (M, w->commands);
		
		mfree (&M->alloc, w);
	}
	
	return NULL;
}

void *FreeForInfo (MGLOBAL *M, FORINFO *f)
{
	if (f)
	{
		FreeWordInfo (M, f->name);
		FreeTreeInfo (M, f->commands);
		FreeCommandInfo (M, f->list);
		
		mfree (&M->alloc, f);
	}
	
	return NULL;
}

void *FreeRegInfo (MGLOBAL *M, REGINFO *reg)
{
	if (reg)
	{
		FreeRegInfo (M, reg->next);
		FreeWordInfo (M, reg->matches);
		FreeTreeInfo (M, reg->commands);
		
		mfree (&M->alloc, reg);
	}
	return NULL;
}

void *FreeCaseInfo (MGLOBAL *M, CASEINFO *c)
{
	if (c)
	{
		FreeWordInfo (M, c->name);
		FreeRegInfo (M, c->case_list);
		
		mfree (&M->alloc, c);
	}
	
	return NULL;
}

void *FreeParInfo (MGLOBAL *M, PARINFO *par)
{
	if (par)
	{
		FreeTreeInfo (M, par->tree);
		
		mfree (&M->alloc, par);
	}
	
	return NULL;
}

void *FreeListInfo (MGLOBAL *M, LISTINFO *list)
{
	if (list)
	{
		FreeTreeInfo (M, list->left);
		FreeTreeInfo (M, list->right);
		
		mfree (&M->alloc, list);
	}
	
	return NULL;
}

void *FreeTreeInfo (MGLOBAL *M, TREEINFO *tree)
{
	void *p;
	
	if (!tree)
		return NULL;

	p = tree;
	switch (tree->typ & COMMASK)
	{
		case TCOM:
			FreeCommandInfo (M, p);
			break;
			
		case TPAR:
			FreeParInfo (M, p);
			break;
			
		case TFIL:
		case TLIST:
		case TAND:
		case TORF:
			FreeListInfo (M, p);
			break;
			
		case TIF:
			FreeIfInfo (M, p);
			break;
			
		case TWHILE:
		case TUNTIL:
			FreeWhileInfo (M, p);
			break;
			
		case TCASE:
			FreeCaseInfo (M, p);
			break;
			
		case TFORK:
			FreeForkInfo (M, p);
			break;
			
		case TFOR:
			FreeForInfo (M, p);
			break;
			
		case TFUNC:
			FreeFuncInfo (M, p);
			break;
		
		default:
			dprintf ("FreeTreeInfo: unbekannter Typ!\n");
			break;
	}

	return NULL;
}


NAMEINFO *MakeNameInfo (MGLOBAL *M, const char *name)
{
	NAMEINFO *n;
	
	n = mcalloc (&M->alloc, 1, sizeof (NAMEINFO));
	if (n)
	{
		size_t len = strlen (name) + 1;
		
		n->name = mmalloc (&M->alloc, len);
		if (!n->name)
		{
			mfree (&M->alloc, n);
			return NULL;
		}
		
		strcpy ((char *)n->name, name);
		
		if (M->shell_flags.export_all)
		{
			n->flags.exported = TRUE;
		}
	}
	
	return n;
}

void *FreeNameInfo (MGLOBAL *M, NAMEINFO *n)
{
	if (n->flags.function)
		FreeFuncInfo (M, n->func);
	
	if (n->val)
		mfree (&M->alloc, n->val);
	
	mfree (&M->alloc, n->name);
	mfree (&M->alloc, n);
	
	return NULL;
}


void *FreeSubstListInfo (MGLOBAL *M, SUBSTLISTINFO *list)
{
	while (list != NULL)
	{
		SUBSTLISTINFO *tmp;
		
		FreeTreeInfo (M, list->tree);
		tmp = list->next;
		mfree (&M->alloc, list);
		list = tmp;
	}
	
	return NULL;
}


