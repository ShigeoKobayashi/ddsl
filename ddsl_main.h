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


 // For DdsProcessor
#define VARIABLE_COUNT()    ((p)->v_count)
#define VARIABLEs()         ((p)->Vars)
#define VARIABLE(i)         (VARIABLEs()[i])
#define STATUS()            ((p)->status)

#define T_COUNT()           ((p)->t_count)
#define TVs()               ((p)->Ts)
#define TV(i)               (TVs()[i])
#define B_COUNT()           ((p)->b_count)
#define B_SIZE(i)           ((p)->f_max[i])   // re-use array (f_max is no more used after block-decomposition)
#define I_COUNT()           ((p)->i_count)
#define IVs()               ((p)->Is)
#define IV(i)               (IVs()[i])

#define F_COUNTs()          ((p)->f_count)
#define F_COUNT(i)          (F_COUNTs()[i])
#define F_MAX_COUNTs()      ((p)->f_max)
#define F_MAX_COUNT(i)      ((p)->f_max[i])
#define FT_MATRIX()         ((p)->Fs)
#define Fs_CONNECTED(i)     (FT_MATRIX()[i])
#define F_CONNECTED(i,j)    (Fs_CONNECTED(i)[j])
#define F_PAIRED(i)         (Fs_CONNECTED(i)[0])
// List of the computation order
#define V_TOP()             ((p)->VFirst)
#define V_END()             ((p)->VEnd)


// For DdsVariable
#define USER_FLAG(v)        ((v)->UFlag)
#define SYS_FLAG(v)         ((v)->SFlag)
#define SET_SFLAG_ON(v,f)   ((v)->SFlag |= (f));
#define SET_SFLAG_OFF(v,f)  ((v)->SFlag &= (~f));
#define IS_SFLAG_OR(v,f)    ((SYS_FLAG((v))&(f))!=0)
#define IS_SFLAG_AND(v,f)   ((SYS_FLAG((v))&(f))==(f))

#define CLEAR_PROC_FLAG(v)  (SYS_FLAG(v) &= ~DDS_PROC_MASK)

#define NAME(v)       ((v)->Name)
#define SCORE(v)      ((v)->score)
#define NEXT(v)       ((v)->next)
#define INDEX(v)      ((v)->index)
#define RHSV_COUNT(v) (IS_SFLAG_OR((v),DDS_FLAG_SET|DDS_SFLAG_FREE)?0:(v)->Nr)
#define RHSVs(v)      ((v)->Rhsvs)
#define RHSV(v,i)     (RHSVs(v)[i])
#define VALUE(v)      ((v)->Value)
#define FUNCTION(v)   ((v)->Function)

#define IS_REQUIRED(v)      IS_SFLAG_OR(v,DDS_FLAG_REQUIRED)
#define IS_DIVIDED(v)       IS_SFLAG_OR(v,DDS_SFLAG_DIVIDED)
#define IS_TARGETED(v)      IS_SFLAG_OR(v,DDS_FLAG_TARGETED)
#define IS_VOLATILE(v)      IS_SFLAG_OR(v,DDS_FLAG_VOLATILE)
#define IS_INTEGRATED(v)    IS_SFLAG_OR(v,DDS_FLAG_INTEGRATED)
#define IS_FREE(v)          IS_SFLAG_OR(v,DDS_SFLAG_FREE)
#define IS_ALIVE(v)         IS_SFLAG_OR(v,DDS_SFLAG_ALIVE)
#define IS_SET(v)           IS_SFLAG_OR(v,DDS_FLAG_SET)
#define IS_CHECKED(v)       IS_SFLAG_OR(v,DDS_SFLAG_CHECKED)
#define IS_STACKED(v)       IS_SFLAG_OR(v,DDS_SFLAG_STACKED)
#define IS_DIVISIBLE(v)     IS_SFLAG_OR(v,DDS_FLAG_DIVISIBLE)
#define IS_NDIVISIBLE(v)    IS_SFLAG_OR(v,DDS_FLAG_NON_DIVISIBLE)
#define IS_DERIVATIVE(v)    IS_SFLAG_OR(v,DDS_SFLAG_DERIVATIVE)

// f is an exception flag (any bit of f is on,then returns 0 or do nothing.
#define BACKWAY_COUNT(v,f)    (IS_SFLAG_OR(v, f) ? 0 : RHSV_COUNT(v))
#define ENABLE_BACKTRACK(f)   {for (int jv = 0; jv < cv; ++jv) INDEX(VARIABLE(jv)) = BACKWAY_COUNT(VARIABLE(jv),f);}
#define MOVE_BACK(v,f)        {if (INDEX(v) >= 0) {while (--INDEX(v) >= 0) {if (!(IS_SFLAG_OR(RHSV(v, INDEX(v)), f))) break;}}}

// Stack operations
#define STACK(n)         vector<DdsVariable*> stack(n)
#define PUSH(v)          stack.push_back(v)
#define PUSH_F(v)        {SET_SFLAG_ON(v,DDS_SFLAG_STACKED);PUSH(v);}
#define POP()            stack.pop_back()
#define PEEK()           stack.back()
#define POP_F()          {SET_SFLAG_OFF(PEEK(),DDS_SFLAG_STACKED);POP();}
#define STACK_CLEAR()    stack.clear()
#define STACK_SIZE()     ((int)stack.size())
#define STACK_ELEMENT(i) stack[i]

#define ENTER(h)   DdsProcessor*p;try{p=(DdsProcessor*)h; STATUS()=0;
#define LEAVE(h)   } catch(Exception& ex) {return STATUS()=ex.Code;}\
                     catch (...) { TRACE_EX(("Aborted by unexpected error!")); return STATUS()=DDS_ERROR_SYSTEM;}


using namespace std;

#endif
