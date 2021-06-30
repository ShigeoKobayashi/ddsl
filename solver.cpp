/*
 *
 * DDSL: Digital Dynamic Simulation Library (C/C++ Library).
 *
 * Copyright(C) 2021 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ddsl.h"
#include "ddsl_main.h"
#include "utils.h"
#include "debug.h"

#define _IX2(i,j)       ((i)*(n)+j)
#define J(i,j)          JACOBIAN_MATRIX()[_IX2(i,j)]
#define UPDATE_F(i,v)   {VALUE(F_PAIRED(ix_top+i)) = v;}
#define UPDATE_XY_I(i)  {X(i)=VALUE(F_PAIRED(ix_top + i));Y(i) = Y_NEXT(i);}
#define UPDATE_XY()     {for (int i = 0; i < n; ++i) { UPDATE_XY_I(i);} }
#define EQUAL(a,b)      (((a == b)||(fabs(a-b)/(fabs(a)+fabs(b)))<1.0e-6)?true:false)
#define CONVERGED()     (norm <= EPS())
#define NOT_CONVERGED() (norm >  EPS())

static int  LuDecomp(DdsProcessor *p,double* A, double* scales, int* index, int n);
static void LuSolve(DdsProcessor* p, double* x, double* LU, double* b, int* index, int n);

static DdsVariable* Newton(DdsProcessor* p, DdsVariable* v_top);
static void         ComputeStatic(DdsProcessor* p, DdsVariable* v_top);
static double       ComputeBlock(DdsProcessor* p, DdsVariable* pv,double *y);
static void         ComputeJacobian(DdsProcessor *p,DdsVariable *pv,int n,int ix_top);
static void         SolveJacobian(DdsProcessor* p, int n);
static void         Derivate(DdsProcessor* p, DdsVariable* pv, int n, int ix_top, int j);

//
//  Set INDEX(IV(i)) = i at the start of DdsComputeDynamic()
//
static double Euler(DdsProcessor* p, DdsVariable* v)
{
    // <I> = <I> + <DR>*STEP()
    return IS(INDEX(v)) + VALUE(RHSV(v,0)) * VALUE(STEP());
}

EXPORT(int) DdsComputeStatic(DDS_PROCESSOR ph)
{
	ENTER(ph);
    if (STAGE() == DDS_COMPUTED_ALL) THROW(DDS_ERROR_COMPUTED, DDS_MSG_COMPUTED);

    int cv = VARIABLE_COUNT();

	if (STAGE() == 0) {
		int m  = 0;

        for (int i = 0; i < I_COUNT(); ++i) {
            if(FUNCTION(IV(i))==nullptr) FUNCTION(IV(i)) = (ComputeVal)Euler;
        }

		// Initialize solver
		for (int i = 0; i < B_COUNT(); ++i) {
			if (m < B_SIZE(i)) m = B_SIZE(i);
		}
        if (m > 0) {
            JACOBIAN_MATRIX() = (double*)MemAlloc(p,sizeof(double) * m * m);
            DELTAs()          = (double*)MemAlloc(p,sizeof(double) * m);  // delta of f(x+delta) = f(x) + (df/dx)delta = 0
            Xs()              = (double*)MemAlloc(p,sizeof(double) * m);
            DXs()             = (double*)MemAlloc(p,sizeof(double) * T_COUNT());  // dx of dy/dx (derivation,total base)
            Ys()              = (double*)MemAlloc(p,sizeof(double) * m);
            Y_NEXTs()         = (double*)MemAlloc(p,sizeof(double) * m);
            SCALE()           = (double*)MemAlloc(p,sizeof(double) * m);
            PIVOT()           = (int   *)MemAlloc(p,sizeof(int) * m);
        }
		STAGE() = DDS_COMPUTED_ONCE;
        // Compute everything at the first.
        ComputeStatic(p, V_TOP_ONCET());
        ComputeStatic(p, V_TOP_EVERYT());
        STAGE() = DDS_COMPUTED_ANY_TIME;
    } else {
        if (STAGE() == DDS_COMPUTED_EVERY_TIME) {
            ComputeStatic(p, V_TOP_EVERYT());
            STAGE() = DDS_COMPUTED_ANY_TIME;
        } else {
            if (STAGE() == DDS_COMPUTED_ANY_TIME) {
                ComputeStatic(p, V_TOP_ANYT());
                STAGE() = DDS_COMPUTED_ALL;
            }
        }
    }
    LEAVE(ph);
	return STATUS();
}

static
void ComputeStatic(DdsProcessor* p, DdsVariable* v_top)
{
    DdsVariable* pv = v_top;
    while(pv != nullptr) {
        if (!IS_FREE(pv)) {VALUE(pv) = FUNCTION(pv)(p, pv); pv = NEXT(pv);}
        else               pv        = Newton(p, pv);
    }
}

static
DdsVariable* Newton(DdsProcessor* p, DdsVariable* vf)
{
    bool   fOk  = false;
    double norm;
    int    ni   = 0;
    if (p->max_iter <= 0) p->max_iter = 1000;
    int    max_iter = p->max_iter;

    ASSERT(IS_FREE(vf));
    int ix_top      = INDEX(vf);        // INDEX to TV() & F_PAIRED()
    int n           = B_SIZE(SCORE(vf) - 1); // block size;
    DdsVariable* pv = NEXT(F_PAIRED(ix_top + n - 1)); // The first variable next to <F>s

    if (EPS() <= 0.0) EPS() = 1.0e-6;

    for (int i = 0; i < n; ++i) X(i) = VALUE(F_PAIRED(ix_top + i));
    norm = ComputeBlock(p, pv,Ys());
    while(NOT_CONVERGED()) {
        // Not yet converged. => Compute Jacobian matrix
        ++ni;
        ComputeJacobian(p,pv,n,ix_top);
        SolveJacobian(p,n);
        // Next estimation
        double fact  = 1.0;
        double ffact = 1.0;
        bool ok = false;
        double dx = 0;
        do {
            bool too_much = false;
            do {
                double DX = 0.0;
                dx        = 0.0;
                double ff = fact * ffact;
                too_much = false;
                for (int i = 0; i < n; ++i) {
                    double d = DELTA(i) * ff;
                    UPDATE_F(i, X(i) + d);
                    dx += fabs(d);
                    DX += fabs(X(i));
                }
                if (ni > 1) {
                    // Non-linear case
                    if (DX < EPS()) DX = EPS();
                    if (dx < EPS()) break;
                    if ((DX / dx) < 0.0001) {
                        // Update too much!!
                        ffact *= 0.1;
                        too_much = true;
                    }
                }
            } while (too_much);

            // try new X()
            double t_norm = ComputeBlock(p, pv,Y_NEXTs());
            if (t_norm < norm) {
                norm = t_norm;
                UPDATE_XY(); ok = true;
                if (CONVERGED()) break;
            } else {
                // unable to move! restore old value.
                for (int i = 0; i < n; ++i) UPDATE_F(i, X(i));
                if (ok) break;
                fact *= 0.1;
            }
        } while ( dx/norm > EPS() );
        if(!ok) {
            // Unable to decrease by Newton -> direct search
            for (int i = 0; i < n; ++i) UPDATE_F(i, X(i));
            for (int i = 0; i < n; ++i) {
                double t_fact = 0.5;
                double dxi = fabs(DELTA(i));
                double xi  = fabs(X(i));
                if (xi < EPS()) xi = EPS();
                while ((xi / (dxi*t_fact)) < 0.0001) t_fact *= 0.1;
                do {
                    UPDATE_F(i, X(i) + DELTA(i) * t_fact);
                    double t_norm = ComputeBlock(p, pv,Y_NEXTs());
                    if (t_norm < norm) {
                        norm = t_norm; ok = true; UPDATE_XY_I(i);
                        break;
                    } else {
                        t_fact *= 0.1;
                        UPDATE_F(i, X(i) - DELTA(i) * t_fact);
                        t_norm = ComputeBlock(p, pv,Y_NEXTs());
                        if (t_norm < norm) {
                            norm = t_norm; ok = true; UPDATE_XY_I(i); break;
                        } else {
                            // unable to move! restore old value.
                            UPDATE_F(i,X(i));
                        }
                    }
                } while (t_fact > EPS());
            }
        }
        if (!ok || ni > max_iter) THROW(DDS_ERROR_CONVERGENCE, DDS_MSG_CONVERGENCE);
    }
    return NEXT(TV(ix_top + n - 1));
}

static 
double ComputeBlock(DdsProcessor* p,DdsVariable* pv,double *y)
{
    ASSERT(!IS_FREE(pv));
    p->current_block = SCORE(pv)-1; // For debugging, save block number to solved.
    double norm = 0.0;
    while (!IS_TARGETED(pv)) {
        VALUE(pv) = FUNCTION(pv)(p, pv);
        pv = NEXT(pv);
    }
    ASSERT(IS_TARGETED(pv));
    do {
        if (IS_DIVIDED(pv)) VALUE(pv) = VALUE(RHSV(pv,RHSV_COUNT(pv)));
        *y = VALUE(pv) - FUNCTION(pv)(p, pv);
        norm += fabs(*y++);
        pv = NEXT(pv);
        if (pv == nullptr) break;
    } while (IS_TARGETED(pv));
	return norm;
}

static void SolveJacobian(DdsProcessor* p,int n)
{
    LuDecomp(p,JACOBIAN_MATRIX(), SCALE(), PIVOT(), n);
    LuSolve(p,DELTAs(), JACOBIAN_MATRIX(), Ys(), PIVOT(), n);
    for (int i = 0; i < n; ++i) DELTA(i) = -DELTA(i);
}

static 
void ComputeJacobian(DdsProcessor* p, DdsVariable* pv, int n, int ix_top)
{
    for (int j = 0; j < n; ++j) {
        Derivate(p, pv, n, ix_top, j);
    }
}

// Computes the derivatives  dTVs/dF[j] (j-th Column of the Jacobian matrix)
static void Derivate(DdsProcessor *p,DdsVariable *pv,int n, int ix_top, int j)
{
    int ok       = 0;
    int ni       = 0;
    double xSave = X(j);
    double dx    = DX(j);

    if (EQUAL(dx, 0.0)) dx = 1.0e-4;

    while (ok <= 0) {
        if (++ni > 100) THROW(DDS_ERROR_JACOBIAN, DDS_MSG_JACOBIAN);
        X(j) = xSave + dx;
        UPDATE_F(j, X(j));
        double norm = ComputeBlock(p, pv,Y_NEXTs());
        if (norm <= EPS() * n) { dx = dx * 5.0; continue; }
        ok = 1;
        for (int i = 0; i < n; ++i)  {
            J(i, j) = (Y_NEXT(i)-Y(i)) / dx;
        }
        DX(j) = dx;
    }
    X(j) = xSave;
    UPDATE_F(j, X(j));
}

 //
 // decompose given matrix A to LU form. L is the lower triangular, and U is the upper triangular matrix.
 // L and U are stored A.
 //  A         ... [IN/OUT], Matrix to be decomposed,and result of decomposition are stored on exit.
 //  scales[n] ... [OUT], work array for pivotting.
 //  index [n] ... [OUT], indices for pivotting/scaling,which is used later in DtxLuSolve()
 //
 // returns 0,  normally decomposed.
 //        -k(negative), failed to decompose when processing A[k-1,*].
 //
static 
int LuDecomp(DdsProcessor *p,double* A, double* scales, int* index, int n)
{
	double biggst;
	double v;
	int pividx = -1;
	int ix;

	for (int i = 0; i < n; ++i) {
		// pick up largest(abs. val.) element in each row.
		index[i] = i;
		biggst = 0.0;
		for (int j = 0; j < n; ++j) {
			v = A[_IX2(i, j)];
			if (v < 0.0) v = -v;
			if (v > biggst) biggst = v;
		}
		if (biggst > 0.0) scales[i] = 1.0 / biggst;
		else {
			ASSERT(false);
            TRACE_EX(("Singular Jacobian at block %d", p->current_block));
            THROW(DDS_ERROR_JACOBIAN, DDS_MSG_JACOBIAN);
			// return -(i + 1); // Singular matrix(case 1): no non-zero elements in row(i) found.
		}
	}
	int n1 = n - 1;
	for (int k = 0; k < n1; ++k) {
		// Gaussian elimination with partial pivoting.
		biggst = 0.0;
		for (int i = k; i < n; ++i) {
			int ip = index[i];
			v = A[_IX2(ip, k)];
			if (v < 0.0) v = -v;
			v = v * scales[ip];
			if (v > biggst) {
				biggst = v;
				pividx = i;
			}
		}
		if (biggst <= 0.0) {
            ASSERT(false);
            TRACE_EX(("Singular Jacobian at block %d", p->current_block));
            THROW(DDS_ERROR_JACOBIAN, DDS_MSG_JACOBIAN);
			// return -(k + 1); // Singular matrix(case 2): no non-zero elements in row(k) found.
		}
		if (pividx != k) {
			int j = index[k];
			index[k] = index[pividx];
			index[pividx] = j;
		}
		int kp = index[k];
		int kp1 = k + 1;
		double pivot = A[_IX2(kp, k)];
		ASSERT(pivot != 0.0);
		for (int i = kp1; i < n; ++i) {
			ix = index[i];
			double mult = A[_IX2(ix, k)] / pivot;
			A[_IX2(ix, k)] = mult;
			if (mult != 0.0) {
				for (int j = kp1; j < n; ++j) A[_IX2(ix, j)] -= mult * A[_IX2(kp, j)];
			}
		}
	}
	if (A[_IX2(index[n1], n1)] == 0.0) {
		ASSERT(false);
        TRACE_EX(("Singular Jacobian at block %d", p->current_block));
        THROW(DDS_ERROR_JACOBIAN, DDS_MSG_JACOBIAN);
        // return -(n1 + 1); // Singular matrix(case 3) at n1.
	}
	return 0;
}

//
// solves linear equation system Ax=b, A(=LU) must be priocessed by LuDecomp() before hand.
//
// index[n] must be the array processed by DtxLuDecomp().
// 
static
void LuSolve(DdsProcessor *p,double* x, double* LU, double* b, int* index, int n)
{
	double dot = 0.0;
	int ix;
	for (int i = 0; i < n; ++i) {
		dot = 0.0;
		ix = index[i];
		for (int j = 0; j < i; ++j) dot += LU[_IX2(ix, j)] * x[j];
		x[i] = b[ix] - dot;
	}

	for (int i = (n - 1); i >= 0; --i) {
		dot = 0.0;
		ix = index[i];
		for (int j = (i + 1); j < n; ++j) dot += LU[_IX2(ix, j)] * x[j];
		ASSERT(LU[_IX2(ix, i)] != 0.0);
		x[i] = (x[i] - dot) / LU[_IX2(ix, i)];
	}
}
