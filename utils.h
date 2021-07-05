/*
 *
 * Utility routines.
 *  (Memory allocation,Exception handling).
 *
 *  Copyright(C) 2021 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */
#ifndef __INC_UTILS__
#define  __INC_UTILS__

#include <exception>
#include "debug.h"

using namespace std;
#define THROW(e,m)  throw(Exception(p,e,m,__FILE__,__LINE__))
class Exception : public exception {
public:
	Exception(DdsProcessor* p, int code, const char* msg, const char* file, int line)
	{
		Code = code;
		if (p != nullptr) {
			(p->ExitStatus).Status = code;
			(p->ExitStatus).Msg = msg;
			(p->ExitStatus).File;
			(p->ExitStatus).Line;
			if (p->ErrorHandler != nullptr) (*p->ErrorHandler)(p);
		}
		TRACE(("\n** Exception(%d,%s) in %s at %d\n",code,msg,file,line));
	};

public:
	int Code;
};

void* MemAlloc(DdsProcessor *p,int s);
void* MemRealloc(DdsProcessor *p,void *pv,int s);
void  MemFree(DdsProcessor *p,void** pp);

#endif
