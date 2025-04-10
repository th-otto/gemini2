/*
 * @(#) MParse\PrntTree.c
 * @(#) Stefan Eissing, 16. September 1991
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "partypes.h"
#include "prnttree.h"


void PrintWordInfo (MGLOBAL *M, WORDINFO *w)
{
	while (w)
	{
		mprintf (M, "%s ", w->name);
		w = w->next;
	}
}

void PrintIoInfo (MGLOBAL *M, IOINFO *io)
{
	while (io)
	{
		if (io->name && io->name->name)
		{
			mprintf (M, "%d", io->file & IO_UFD);
			
			if (io->file & IO_DOC)
				mprintf (M, "<<");
			else if (io->file & IO_MOV)
			{
				if (io->file & IO_PUT)
					mprintf (M, ">&");
				else
					mprintf (M, "<&");
			}
			else if ((io->file & IO_PUT) == 0)
				mprintf (M, "<");
			else if (io->file & IO_APP)
				mprintf (M, ">>");
			else
				mprintf (M, ">");
			
			mprintf (M, "%s ", io->name->name);
		}
	
		io = io->next;
	}
}

void PrintTreeInfo (MGLOBAL *M, TREEINFO *tree)
{
	void *p;
	
	if (!tree)
		return;

	p = tree;
	switch (tree->typ & COMMASK)
	{
		case TCOM:
		{
			COMMANDINFO *c = p;
			
			PrintWordInfo (M, c->set);
			PrintWordInfo (M, c->args);
			PrintIoInfo (M, c->io);
			break;
		}
			
		case TPAR:
		{
			PARINFO *par = p;
			
			mprintf (M, "( ");
			PrintTreeInfo (M, par->tree);
			mprintf (M, " )");
			break;
		}
			
		case TFIL:
		{
			LISTINFO *l = p;
			
			PrintTreeInfo (M, l->left);
			mprintf (M, " | ");
			PrintTreeInfo (M, l->right);
			break;
		}
		
		case TLIST:
		{
			LISTINFO *l = p;
			
			PrintTreeInfo (M, l->left);
			mprintf (M, "\n");
			PrintTreeInfo (M, l->right);
			break;
		}

		case TAND:
		{
			LISTINFO *l = p;
			
			PrintTreeInfo (M, l->left);
			mprintf (M, " && ");
			PrintTreeInfo (M, l->right);
			break;
		}

		case TORF:
		{
			LISTINFO *l = p;
			
			PrintTreeInfo (M, l->left);
			mprintf (M, " || ");
			PrintTreeInfo (M, l->right);
			break;
		}
			
		case TIF:
		{
			IFINFO *i = p;
			
			mprintf (M, "if ");
			PrintTreeInfo (M, i->condition);
			mprintf (M, "\nthen\n");
			PrintTreeInfo (M, i->then_case);
			
			if (i->else_case)
			{
				mprintf (M, "\nelse\n");
				PrintTreeInfo (M, i->else_case);
			}
			mprintf (M, "\nfi");
			break;
		}
			
		case TWHILE:
		case TUNTIL:
		{
			WHILEINFO *w = p;
			
			if (w->typ == TWHILE)
				mprintf (M, "while ");
			else
				mprintf (M, "until ");
			PrintTreeInfo (M, w->condition);
			mprintf (M, "\ndo\n");
			PrintTreeInfo (M, w->commands);
			mprintf (M, "\ndone");
			break;
		}
			
		case TCASE:
		{
			CASEINFO *c = p;
			REGINFO *r = c->case_list;
			
			mprintf (M, "case %s in\n", c->name);
			
			while (r)
			{
				WORDINFO *w = r->matches;
				
				if (w)
				{
					mprintf (M, "%s", w->name);
					w = w->next;
				}
			
				while (w)
				{
					mprintf (M, "|%s", w->name);
					w = w->next;
				}
				
				mprintf (M, ") ");
				PrintTreeInfo (M, r->commands);
				mprintf (M, ";;\n");
				
				r = r->next;
			}
			
			mprintf (M, "esac");
			break;
		}
			
		case TFORK:
		{
			FORKINFO *f = p;
			
			PrintTreeInfo (M, f->tree);
			PrintIoInfo (M, f->io);
			if (f->typ & FAMP)
				mprintf (M, " &");
			break;
		}
			
		case TFOR:
		{
			FORINFO *f = p;
			
			mprintf (M, "for %s", f->name->name);
			if (f->list)
			{
				mprintf (M, " in ");
				PrintWordInfo (M, f->list->args);
			}
			
			mprintf (M, "\ndo\n");
			PrintTreeInfo (M, f->commands);
			mprintf (M, "\ndone");
			break;
		}
			
		case TFUNC:
		{
			FUNCINFO *f = p;
			mprintf (M, "%s() {\n", f->name->name);
			PrintTreeInfo (M, f->commands);
			mprintf (M, "\n}");
			break;
		}
		
		default:
			dprintf ("FreeTreeInfo: unbekannter Typ!\n");
			break;
	}

	return;
}

