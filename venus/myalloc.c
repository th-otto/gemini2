/*
 * @(#) Gemini\MyAlloc.c
 * @(#) Stefan Eissing, 25. Februar 1992
 */

#include <stddef.h>
#include <string.h>
#include <aes.h>
#include <tos.h>

#include "..\common\alloc.h"

#include "myalloc.h"

static ALLOCINFO *main_alloc;

void InitMainMalloc (void *alloc_info)
{
	main_alloc = alloc_info;
}

void ExitMainMalloc (void)
{
	main_alloc = NULL;
}

void *malloc (size_t nbytes)
{
	return mmalloc (main_alloc, nbytes);
}

void *realloc (void *block, size_t new_size)
{
	return mrealloc (main_alloc, block, new_size);
}

void *calloc(size_t items, size_t size)
{
	return mcalloc (main_alloc, items, size);
}

void free (void *p)
{
	mfree (main_alloc, p);
}

