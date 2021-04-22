/*
 *
 * Utility routines.
 *  (Memory allocation,Exception handling).
 *
 *  Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include "ddsl.h"
#include "debug.h"
#include "utils.h"

using namespace std;
static int g_alloc_count = 0;

void* MemAlloc(int s)
{
	void* p = calloc(s, sizeof(char));
	if (p == nullptr) THROW(DDS_ERROR_MEMORY, DDS_MSG_MEMORY);
	g_alloc_count++;
	return p;
}

void* MemRealloc(void* p, int s)
{
	void* t = realloc(p, s);
	if (t == nullptr) {
		free(p);
		THROW(DDS_ERROR_MEMORY, DDS_MSG_MEMORY);
	}
	return t;
}

void MemFree(void** pp) 
{
	if ( pp == nullptr) return;
	if (*pp == nullptr) return;
	free(*pp);
	g_alloc_count--;
	*pp = nullptr;
}

#ifdef _DEBUG
void PrintAllocCount(const char *msg)
{
	printf(" \n%s:Memory allocation count=%d\n",msg, g_alloc_count);
}
#endif
