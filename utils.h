/*
 *
 * Utility routines.
 *  (Memory allocation,Exception handling).
 *
 *  Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */
#ifndef __INC_UTILS__
#define  __INC_UTILS__

#include <exception>
#include "debug.h"

using namespace std;
#define THROW(e,m)  throw(Exception(e,m,__FILE__,__LINE__))
class Exception : public exception {
public:
	Exception(int code,const char *msg,const char *file,int line)
	{
		Code = code;
		Msg  = msg;
		File = file;
		Line = line;
		TRACE(("\n** Exception(%d,%s) in %s at %d\n",code,msg,file,line));
	};

public:
	const char* Msg;
	const char* File;
	int         Line;
	int         Code;
};

void* MemAlloc(int s);
void* MemRealloc(void *p,int s);
void  MemFree(void** pp);

#endif
