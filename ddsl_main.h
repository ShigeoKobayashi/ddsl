/*
 *
 * DDSL main routines.
 *
 *  Copyright(C) 2021 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */
#ifndef __INC_DDSL_MAIN__
#define  __INC_DDSL_MAIN__

#include <vector>
#include <math.h>
#include "ddsl.h"
#include "debug.h"

 // For DdsProcessor
#define TIME()              ((p)->Time)       // Time
#define STEP()              ((p)->Step)       // Step
#define RHSV_EX()           ((p)->rhsv_extra) // extra elements of RHSVs of eac variable(normally zero).
#define METHOD()            ((p)->method)     // Integration method or steady-state.
#define I_BACKTRACK()       ((p)->i_backtrack) // 0 for BW=EULER method,DDS_FLAG_INTEGRATION for other methods.
#define VARIABLE_COUNT()    ((p)->v_count)    // number of the total variables registered.
#define VARIABLEs()         ((p)->Vars)       // array of the registered variables registered.
#define VARIABLE(i)         (VARIABLEs()[i])  // i-th variable registered.
#define STATUS()            ((p)->status)     // final status of eache function call.
#define JACOBIAN_MATRIX()   ((p)->jacobian)   // Jacobian matrix for solving system of non-linear equations.
#define SCALE()             ((p)->scale)      // an array for scaling value used in LU decomposition
#define PIVOT()             ((p)->pivot)      // for pivoting 
#define T_VALUEs()          ((p)->t_computed)
#define T_VALUE(i)          ((T_VALUEs())[i])
#define DELTAs()            ((p)->delta)
#define DELTA(i)            ((DELTAs())[i])  // next increment each Newton step (block base).
#define Ys()                ((p)->y_cur)     // current value of <T>-value - value computed. (block base) 
#define Y_NEXTs()           ((p)->y_next)    // next value of <T>-value - value computed. (block base)
#define Y(i)                ((Ys())[i])      // Lhsv of y = f(x) ==> 0
#define Y_NEXT(i)           ((Y_NEXTs())[i]) // next estimation of y.
#define Xs()                ((p)->x)
#define X(i)                ((Xs())[i])        // y=f(x) ==> 0
#define DXs()               ((p)->dx)
#define DX(i)               ((DXs())[ix_top+i]) // total base dx (dx of dy/dx)
#define EPS()               ((p)->eps)

#define T_COUNT()           ((p)->t_count)    // Total number of <T>s registered.
#define TVs()               ((p)->Ts)         // Array for all <T>s
#define TV(i)               (TVs()[i])        // i-th <T> variable.
#define B_COUNT()           ((p)->b_count)    // block count
#define B_SIZE(i)           ((p)->f_max[i])   // size of i-th block(re-used array ,f_max is no more used after block-decomposition)
#define I_COUNT()           ((p)->i_count)    // <I> count
#define IVs()               ((p)->Is)         // Array for all <I>s
#define IV(i)               (IVs()[i])        // i-th <I> variable.
#define ISs                 ((p)->R_IVS)      // Value of <I>s before integration.
#define IS(i)               (ISs[i])
#define VtoDRs()            ((p)->I_toDR) 
#define VtoDR(i)            (VtoDRs()[i])

#define F_COUNTs()          ((p)->f_count)
#define F_COUNT(i)          (F_COUNTs()[i])   // number of <F>s connected to <T>[i]
#define F_MAX_COUNTs()      ((p)->f_max)
#define F_MAX_COUNT(i)      ((p)->f_max[i])   // max-counter for <F>s connected to <T>[i] 
#define FT_MATRIX()         ((p)->Fs)
#define Fs_CONNECTED(i)     (FT_MATRIX()[i])
#define F_CONNECTED(i,j)    (Fs_CONNECTED(i)[j]) // j-th <F> connected to <T>[i]
#define F_PAIRED(i)         (Fs_CONNECTED(i)[0]) // <F> paired with <T>[i]
// List of the computation order
#define V_TOP_ONCET()       ((p)->VTopOnceT)
#define V_END_ONCET()       ((p)->VEndOnceT)
#define V_TOP_ANYT()        ((p)->VTopAnyT)
#define V_END_ANYT()        ((p)->VEndAnyT)
#define V_TOP_EVERYT()      ((p)->VTopEveryT)
#define V_END_EVERYT()      ((p)->VEndEveryT)
#define STAGE()             ((p)->stage)

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
