/*
 * @(#) Gemini\myalloc.h
 * @(#) Stefan Eissing, 25. Februar 1992
 *
 * description: Header File for myalloc.c
 */

#ifndef G_myalloc__
#define G_myalloc__

void InitMainMalloc (void *alloc_info);
void ExitMainMalloc (void);

void *malloc (size_t nbytes);
void *realloc (void *block, size_t new_sizw);
void *calloc(size_t items, size_t size);
void free (void *p);

#endif