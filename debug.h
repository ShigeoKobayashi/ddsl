/*
 *
 * Debugging tools.
 *
 * Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */
#ifndef __INC_DEBUG__
#define  __INC_DEBUG__

#ifdef _DEBUG
#define ASSERT(f)      {if(!(f)) printf("++ ASSERTION failed[%s/%d] ++\n",__FILE__,__LINE__);}
#define ASSERT_EX(f,s) {if(!(f)) printf("++ ASSERTION failed(");printf s,printf(")[%s/%d] ++\n",__FILE__,__LINE__);}
#define TRACE(s)       {printf s;}
#define TRACE_EX(s)    {printf("\n--TRACE:%s/%d[",__FILE__,__LINE__);printf s;printf("]--\n");}
#define _D(ex)         {ex;}
#else
#define ASSERT(f)
#define ASSERT_(f,s)
#define TRACE(s)
#define TRACE_EX(s)
#define _D(ex)
#endif

#endif
