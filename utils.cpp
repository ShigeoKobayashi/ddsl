/*
 *
 * Utility routines.
 *  (Memory allocation,Exception handling).
 *
 *  Copyright(C) 2021 by Shigeo Kobayashi(shigeo@tinyforest.jp).
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

void* MemAlloc(DdsProcessor *p,int s)
{
	void* pv = calloc(s, sizeof(char));
	if (pv == nullptr) THROW(DDS_ERROR_MEMORY, DDS_MSG_MEMORY);
	g_alloc_count++;
	return pv;
}

void* MemRealloc(DdsProcessor *p,void* pv, int s)
{
	void* t = realloc(pv, s);
	if (t == nullptr) {
		free(pv);
		THROW(DDS_ERROR_MEMORY, DDS_MSG_MEMORY);
	}
	return t;
}

void MemFree(DdsProcessor *p,void** pp) 
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
