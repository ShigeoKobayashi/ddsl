/*
 *
 * DDSL main routines.
 *
 *  Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */
#ifndef __INC_DDSL_MAIN__
#define  __INC_DDSL_MAIN__

#include <vector>
#include "ddsl.h"
#include "debug.h"

#define ENTER(h)   DdsProcessor*p;try{p=(DdsProcessor*)h; STATUS(p)=0;
#define LEAVE(h)   } catch(Exception& ex) {return STATUS(p)=ex.Code;}\
                     catch (...) { TRACE_EX(("DdsAddVariableA() aborted")); return STATUS(p)=DDS_ERROR_SYSTEM;}

 // For DdsProcessor
#define VARIABLE_COUNT(p)    ((p)->v_count)
#define T_COUNT(p)           ((p)->t_count)
#define VARIABLE(p,i)        ((p)->Vars[i])
#define STATUS(p)            ((p)->status)

// For DdsVariable
#define USER_FLAG(v)        v->UFlag
#define SYS_FLAG(v)         v->SFlag
#define SET_SFLAG_ON(v,f)   v->SFlag |= f;
#define SET_SFLAG_OFF(v,f)  v->SFlag &= (~f);

#define SCORE(v)      ((v)->score)
#define NEXT(v)       ((v)->next)
#define INDEX(v)      ((v)->index)
#define RHSV_COUNT(v) ((v)->Nr)
#define RHSV(v,i)     ((v)->Rhsvs[i])

#define IS_REQUIRED(v)      DDS_FLAG_OR(SYS_FLAG(v),DDS_FLAG_REQUIRED)
#define IS_DIVIDED(v)       DDS_FLAG_OR(SYS_FLAG(v),DDS_SFLAG_DIVIDED)
#define IS_TARGETED(v)      DDS_FLAG_OR(SYS_FLAG(v),DDS_FLAG_TARGETED|DDS_SFLAG_DIVIDED)
#define IS_VOLATILE(v)      DDS_FLAG_OR(SYS_FLAG(v),DDS_FLAG_VOLATILE)
#define IS_INTEGRATED(v)    DDS_FLAG_OR(SYS_FLAG(v),DDS_FLAG_INTEGRATED)
#define IS_FREE(v)          DDS_FLAG_OR(SYS_FLAG(v),DDS_SFLAG_FREE|DDS_SFLAG_DIVIDED)
#define IS_ALIVE(v)         DDS_FLAG_OR(SYS_FLAG(v),DDS_SFLAG_ALIVE)
#define IS_SET(v)           DDS_FLAG_OR(SYS_FLAG(v),DDS_FLAG_SET)
#define IS_CHECKED(v)       DDS_FLAG_OR(SYS_FLAG(v),DDS_SFLAG_CHECKED)
#define IS_STACKED(v)       DDS_FLAG_OR(SYS_FLAG(v),DDS_SFLAG_STACKED)

// Stack operations
#define STACK(n)    vector<DdsVariable*> stack(n);\
    auto  BACKWAY_COUNT = [](DdsVariable* v) {\
        if (DDS_FLAG_OR(SYS_FLAG(v), DDS_FLAG_SET|DDS_SFLAG_FREE|DDS_FLAG_INTEGRATED)) return 0;\
        return RHSV_COUNT(v);};\
    auto  MOVE_BACK = [](DdsVariable* v) -> DdsVariable* {\
        while (INDEX(v) >= 0) {int ix=--INDEX(v);if(ix>=0 && !(DDS_FLAG_OR(SYS_FLAG(RHSV(v,ix)),DDS_FLAG_TARGETED|DDS_SFLAG_DIVIDED))) return RHSV(v,ix);}\
        return (DdsVariable*)nullptr;};\
    auto  ENABLE_BACKTRACK = [&p,&cv,&BACKWAY_COUNT]() {for(int jv=0;jv<cv;++jv) INDEX(VARIABLE(p, jv))=BACKWAY_COUNT(VARIABLE(p, jv));};

#define PUSH(v)          stack.push_back(v)
#define PUSH_F(v)        {DDS_FLAG_ON(SYS_FLAG(v),DDS_SFLAG_STACKED);PUSH(v);}
#define POP()            stack.pop_back()
#define PEEK()           stack.back()
#define POP_F()          {DDS_FLAG_OFF(SYS_FLAG(PEEK()),DDS_SFLAG_STACKED);POP();}
#define STACK_CLEAR()    stack.clear()
#define STACK_SIZE()     ((int)stack.size())
#define STACK_ELEMENT(i) stack[i]

using namespace std;

#endif
