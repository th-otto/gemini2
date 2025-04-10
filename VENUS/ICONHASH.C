/*
 * @(#) Gemini\iconhash.c
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 */

#include <string.h>
#include <ctype.h>
#include <aes.h>

#include "vs.h"

#include "iconhash.h"
#include "iconrule.h"


/* L„nge der Hashtabelle
 */
#define HASHMAX	37L

/* Die Tabelle selbst */
static DisplayRule *hashTable[HASHMAX];

/* Dies ist die Funktion hashpjw aus dem Aho, Sethi, Ullman:
 * Compilers, S. 436
 */
static word hashValue (const char *name)
{
	register unsigned long h = 0L, g;
	const char *p = name; 
	
	while(*p)
	{
		h = (h << 4) + tolower (*p++);
		if ((g = h & 0xf0000000L) != 0L)
		{
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}

	return (word)(h % HASHMAX);
}


static void hashRule (DisplayRule *pd)
{
	DisplayRule *cd;
	word hash;
	
	pd->wasHashed = TRUE;
	hash = hashValue(pd->wildcard);
	
	cd = hashTable[hash];
	hashTable[hash] = pd;
	pd->nextHash = cd;
}


void BuiltIconHash (void)
{
	DisplayRule *firstRule, *pd;
	
	ClearHashTable ();
	if ((firstRule = GetFirstDisplayRule ()) != NULL)
	{
		for (pd = firstRule; pd != NULL; pd = pd->nextrule)
		{
			pd->nextHash = NULL;
			pd->wasHashed = FALSE;
		}
				
		for (pd = firstRule; pd != NULL; pd = pd->nextrule)
		{
			if (strcspn (pd->wildcard, "*?[],;") 
				== strlen (pd->wildcard))
			{
				hashRule (pd);
			}
		}
	}
}

DisplayRule *GetHashedRule (word isFolder, const char *name, 
				int case_sensitiv)
{
	DisplayRule *pd;
	word hash;
	int (* unequal)(const char *s1, const char *s2);
	
	hash = hashValue (name);
	pd = hashTable[hash];
	unequal = case_sensitiv? strcmp : stricmp;

	while (pd)
	{
		if (!(!isFolder ^ !pd->isFolder)
			&& !unequal (name, pd->wildcard))
		{
			return pd;
		}
		
		pd = pd->nextHash;
	}

	return NULL;
}

void ClearHashTable (void)
{
	register word i;
	
	for (i = 0; i < HASHMAX; i++)
		hashTable[i] = NULL;
}