/*
 * @(#) Gemini\select.h
 * @(#) Stefan Eissing, 29. Januar 1995
 *
 * description: Header File for select.c
 */

#ifndef G_select__
#define G_select__

int ThereAreSelected (WindInfo **wpp);

int SelectObject (WindInfo *wp, word obj, int draw);
int SelectAll (WindInfo *wp);
word SelectAllInTopWindow (void);
const char *StringSelect (WindInfo *wp, char *pattern, word redraw);

int DeselectObject (WindInfo *wp, word obj, int draw);
void DeselectAll (WindInfo *wp);
void DeselectAllExceptWindow (WindInfo *wp);
void DeselectAllObjects (void);

int GetOnlySelectedObject (WindInfo **wpp, word *objnr);

void MarkDraggedObjects (WindInfo *wp);
void UnMarkDraggedObjects (WindInfo *wp);

int ObjectWasDragged (OBJECT *po);
void MarkDraggedObject (WindInfo *wp, OBJECT *po);
void UnMarkDraggedObject (WindInfo *wp, OBJECT *po);

void DeselectNotDraggedObjects (WindInfo *wp);

char *WhatIzIt (word x, word y, word *ob_type, word *owner_id);

#endif	/* !G_select__ */