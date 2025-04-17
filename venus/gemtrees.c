/*
 * @(#) Gemini\gemtrees.c
 * @(#) Stefan Eissing, 15. Juli 1991
 *
 * description: functions to work on AES trees
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "myalloc.h"
#include "gemtrees.h"


#define NO_OBJ		-1

/*
 * internals
 */
static word retcode;


/*
 * void walkGemTree(OBJECT *po, word objnr,
 *			void (*walkfunc)(OBJECT *po,word objnr))
 *
 * a simple depth-first walk algorithm for
 * AES-trees
 */
void walkGemTree(OBJECT *po, word objnr,
				void (*walkfunc)(OBJECT *po,word objnr))
{
	word i;
	
	if(po[objnr].ob_head != NO_OBJ)
	{
		for (i=po[objnr].ob_head; (i != objnr); i = po[i].ob_next)
		{
			walkGemTree(po,i,walkfunc);
		}
	}
	walkfunc(po,objnr);		/* and do it for yourself */
}



/* Richtung, in der sortiert werden soll
 */
static word sortgrav1, sortgrav2;

static word cmpObjects(OBJECT **po1, OBJECT **po2)
{
	word xdiff, ydiff;
	word first, second;
	
	ydiff = (*po1)->ob_y - (*po2)->ob_y;
	xdiff = (*po1)->ob_x - (*po2)->ob_x;

	switch (sortgrav1)
	{
		case D_SOUTH:
			ydiff = -ydiff;
		case D_NORTH:
			first = ydiff;
			if (sortgrav2 == D_EAST)
				xdiff = -xdiff;
			second = xdiff;
			break;

		case D_EAST:
			xdiff = -xdiff;
		case D_WEST:
			first = xdiff;
			if (sortgrav2 == D_SOUTH)
				ydiff = -ydiff;
			second = ydiff;
			break;
	}
	
	if (first)
		return first;
	else
		return second;
}

/*
 * sortiert die Objekte in tree
 * startet mit objekt startobj und geht
 * maxlevel tief
 */
word sortTree(OBJECT *tree, word startobj, word maxlevel, 
					word grav1, word grav2)
{
	OBJECT **objlist;
	word i,nr,anz,*lastptr;
	ptrdiff_t objnr;

	sortgrav1 = grav1;
	sortgrav2 = grav2;
	
	if ((maxlevel < 1) || (tree[startobj].ob_head == NO_OBJ))
		return TRUE;
		
	anz = 0;
	for(nr=tree[startobj].ob_head;
		nr != startobj; nr=tree[nr].ob_next)
		++anz;

	objlist = malloc(anz * sizeof(OBJECT *));
	if (!objlist)
		return FALSE;
	
	for(nr=tree[startobj].ob_head,i=0;
		nr != startobj; nr=tree[nr].ob_next, ++i)
	{
		if (tree[nr].ob_head != NO_OBJ)
			sortTree(tree,nr,maxlevel-1, grav1, grav2);
			
		objlist[i] = &tree[nr];
	}

	qsort (objlist, anz, sizeof (OBJECT *), cmpObjects);

	lastptr = &tree[startobj].ob_head;
	
	for(i=0; i<anz; ++i)
	{
		objnr = objlist[i] - tree;
		*lastptr = (word)objnr;
		lastptr = &tree[objnr].ob_next;
	}
	*lastptr = startobj;
	tree[startobj].ob_tail = (word)objnr;
	free(objlist);
	return TRUE;
}


static word calcChilds (OBJECT *tree, word index, word *height)
{
	word anz;
	word last, last_index;
	
	if (tree[index].ob_head < 0)
		return 0;
	
	last = tree[index].ob_tail;
	index = tree[index].ob_head;
	*height = tree[index].ob_height;
	anz = 1;
	
	while (index != last)
	{
		last_index = index;
		index = tree[index].ob_next;
		if (tree[index].ob_y != tree[last_index].ob_y)
		{
			*height += tree[index].ob_height;
			++anz;
		}
	}
	
	return anz;
}

static void fixInButton(OBJECT *tree, word index)
{
	word childs, last, h, y, step, last_y;
	
	if (tree[index].ob_type != (0x1400|G_USERDEF))
		return;
	
	childs = calcChilds(tree, index, &h);
	if (childs)
	{
		step = (tree[index].ob_height - h - HandYSize) / (childs+1);
		if (step <= 0)
			return;
		
		y = step + HandYSize;
		last = tree[index].ob_tail;
		index = tree[index].ob_head;
		last_y = tree[index].ob_y;
		tree[index].ob_y = y;
		
		while (last != index)
		{
			index = tree[index].ob_next;
			if (last_y != tree[index].ob_y)
				y += step + tree[index].ob_height;
			last_y = tree[index].ob_y;
			tree[index].ob_y = y;
		}
	}
}

void FixTree(OBJECT *tree)
{
	ObjcTreeInit(tree);
	walkGemTree(tree, 0, fixInButton);
}

static void getMaxNo(OBJECT *tree, word index)
{
	(void)tree;
	if (index > retcode)
		retcode = index;
}

/* markiere das letzte Objekt in einem Baum mit
 * LASTOB.
 */
void SetLastObject(OBJECT *tree)
{
	word i;
	
	retcode = 0;
	walkGemTree(tree, 0, getMaxNo);
	
	for (i = 0; i < retcode; ++i)
		tree[i].ob_flags &= ~LASTOB;
	
	tree[retcode].ob_flags |= LASTOB;
}