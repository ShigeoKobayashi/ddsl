/*
 *
 * DDSL: Digital Dynamic Simulation Library (C/C++ Library).
 *
 * Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
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

//
// Select <AL>ive variables that have contribution to compute any <R>,and define connected components.
// Also checks number of <F>s==<T>s.
//
// On return:
//   DDS_SFLAG_ERROR on any variable means which has contradicted user-flags and conputation can't continue.
//   If it is not set on any variable,then
//     SCORE(v)   ... Connected component no.,the v icluded. SCORE(v)==0 means v is not alive.
//     NEXT(v)    ... Circular link that links every variables in the same connected component.
//     T_COUNT(p) ... Total number of <T>s.
//
EXPORT(int) DdsSieveVariable(DDS_PROCESSOR ph)
{
	ENTER(ph);
	int cv = VARIABLE_COUNT(p);
	STACK(cv+1);
	{
		// Check and copy user-flag to system-flag.
		int ce = 0;
		for (int i = 0; i < cv; ++i) {
			SYS_FLAG(VARIABLE(p, i)) = USER_FLAG(VARIABLE(p, i)) & DDS_FLAG_MASK;
			if(DdsCheckVariable(VARIABLE(p, i))) ++ce;
		}
		if (ce > 0) THROW(DDS_ERROR_FLAG, DDS_MSG_FLAG);
	}

	// can back-track from <I> to <DE>.
	auto  BACKWAY_COUNTI = [](DdsVariable* v) {
		if (DDS_FLAG_OR(SYS_FLAG(v), DDS_FLAG_SET | DDS_SFLAG_FREE)) return 0;
		return RHSV_COUNT(v);
	};

	auto  ENABLE_BACKTRACKI = [&p, &cv,&BACKWAY_COUNTI]() {
		for (int jv = 0; jv < cv; ++jv) INDEX(VARIABLE(p, jv)) = BACKWAY_COUNTI(VARIABLE(p, jv));
	};

	auto MERGE = [](DdsVariable* v1, DdsVariable* v2) {
		int s1 = SCORE(v1);
		int s2 = SCORE(v2);
		int s  = s1;
		DdsVariable* v = v2;
		if (s1 == s2) return;
		if (s1 > s2) {
			s = s2;
			v = v1;
		}
		DdsVariable* v_start = v;
		do {
			SCORE(v) = s;
		} while ((v = NEXT(v)) != v_start);
		v = NEXT(v1);
		NEXT(v1) = NEXT(v2);
		NEXT(v2) = v;
	};
	
	// Find <R> and setup <F> flag. And some more checks.
	int ct = 0; // <T> counter
	{
		int cr = 0; // <R> counter
		int cf = 0; // <F> counter
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(p, i);
			if (pv == nullptr) THROW(DDS_ERROR_NULL, DDS_MSG_NULL);
			SCORE(pv) = 0;
			if (RHSV_COUNT(pv) <= 0 && !IS_SET(pv)) {++cf; SET_SFLAG_ON(pv, DDS_SFLAG_FREE);}
			if (IS_REQUIRED(pv)) ++cr;
			if (IS_TARGETED(pv)) ++ct;
			for (int j = 0; j < RHSV_COUNT(pv); ++j) {
				if (RHSV(pv, j) == nullptr) THROW(DDS_ERROR_NULL, DDS_MSG_NULL);
			}
		}
		if (cr <= 0 ) THROW(DDS_ERROR_REQUIRED,  DDS_MSG_REQUIRED);
		if (cf != ct) THROW(DDS_ERROR_FT_NUMBER, DDS_MSG_FT_NUMBER);
	}
	T_COUNT(p) = 0;
	if (ct > 0) {
		// ** Make <F>-T<> circular link. **
		//    NEXT(v):Any variable,except starting <T>, on <T>-<F> route points starting <T> 
		//            and NEXT(<T>) constructs circular link of equation system.
		//
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(p, i);
			SCORE(pv) = i + 1;
			if (IS_TARGETED(pv)) NEXT(pv) = pv;      //  <T> => self loop
			else                 NEXT(pv) = nullptr; // !<T>
		}
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(p, i);
			if (!IS_TARGETED(pv)) continue;
			ENABLE_BACKTRACK();
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			while ((pv = PEEK()) != nullptr) {
				if (!IS_FREE(pv)) {
					// go-back further. but not go-back over <T>.
					DdsVariable *pNext = MOVE_BACK(pv);
					if (pNext != nullptr) PUSH(pNext);
					else                  POP();
				} else {
					// reached to <F>.
					DdsVariable* T = STACK_ELEMENT(1);
					for (int i = 2; i < STACK_SIZE(); ++i)
					{
						if (NEXT(STACK_ELEMENT(i)) != nullptr) MERGE(T, NEXT(STACK_ELEMENT(i)));
						else NEXT(STACK_ELEMENT(i)) = T;
					}
					POP(); // POP(<F>)
				}
			}
		}
		// Back-track from <R> to <T>-<F> route.If it reaches the route,then set <R> to <T>s on the route.
		// This back-track over <I>.
		ENABLE_BACKTRACKI();
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(p, i);
			if (!IS_REQUIRED(pv)) continue;
			if ( IS_TARGETED(pv)) continue;
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			while ((pv = PEEK()) != nullptr) {
				if (NEXT(pv) != nullptr) {
					// reached to <T>-<F> route.
					DdsVariable* T = NEXT(pv);
					pv = T;
					do {
						SET_SFLAG_ON(T, DDS_FLAG_REQUIRED);
					} while ((T = NEXT(T)) != pv);
				}
				// Back-track
				DdsVariable* pNext = MOVE_BACK(pv);
				if (pNext != nullptr) PUSH(pNext);
				else                  POP();
			}
		}
	}
	//
	// Set <AL> to every variables that have contribution to compute <R>(<R> itself is <AL>).
	// This back-track over <I>.
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		if (!IS_REQUIRED(pv)) continue;
		SET_SFLAG_ON(pv, DDS_SFLAG_ALIVE);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		while ((pv = PEEK()) != nullptr) {
			ASSERT(IS_ALIVE(pv));
			POP();
			if (NEXT(pv) != nullptr && !IS_REQUIRED(NEXT(pv))) {
				// reached to not-yet-processed <F>-<T> route.
				DdsVariable* pStart = NEXT(pv);
				DdsVariable* rv = pStart;
				do {
					DDS_FLAG_ON(SYS_FLAG(rv), DDS_FLAG_REQUIRED);;
				} while ((rv = NEXT(rv)) != pStart);
			}
			for (int j = 0; j < RHSV_COUNT(pv); ++j) {
				DdsVariable* pNext = RHSV(pv, j);
				SET_SFLAG_ON(pNext, DDS_SFLAG_ALIVE);
				if (DDS_FLAG_OR(SYS_FLAG(pNext), DDS_FLAG_SET | DDS_FLAG_TARGETED)) continue;
				PUSH(pNext);
			}
		}
	}
	// If all RHSVs of any <T>'s are not <AL>,the make the <T> to <S>.
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		if (IS_TARGETED(pv)) {
			if (!IS_ALIVE(RHSV(pv, 0))) {
#ifdef _DEBUG
				for (int j = 1; j < RHSV_COUNT(pv); ++j) ASSERT(!IS_ALIVE(RHSV(pv, j)));
#endif
				DDS_FLAG_OFF(SYS_FLAG(pv), DDS_FLAG_TARGETED);
				DDS_FLAG_ON(SYS_FLAG(pv), DDS_FLAG_SET);
			}
		}
	}
	//
	// define connected components
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		NEXT(pv)  = pv;
		if (IS_ALIVE(pv)) SCORE(pv) = i + 1;
		else              SCORE(pv) = 0;     // !<AL>'s component no. = 0
	}
	// Merge variables in the same connected component.
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		if (!IS_ALIVE(pv)) continue;
		int nr = BACKWAY_COUNT(pv); // Not back-track(MERGE) over <I>
		for(int j=0;j<nr;++j) {
			DdsVariable* rVar = RHSV(pv,j);
			ASSERT(IS_ALIVE(rVar));
			MERGE(pv,rVar);
		}
	}
	// Count total number of connected components and renumber them
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		SET_SFLAG_OFF(pv, DDS_SFLAG_CHECKED);
	}
	int nc = 0; // Number of current connected component.
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		if (!IS_ALIVE(pv) ) continue;
		if (IS_CHECKED(pv)) continue;
		++nc;
		DdsVariable* pvStart = pv;
		do {
			SCORE(pv) = nc;
			SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		} while ((pv = NEXT(pv)) != pvStart);
	}
	TRACE_EX((" %d components ",nc));
	vector<int> cF(nc);
	vector<int> cT(nc);
	for (int i = 0; i < nc; ++i) {
		cF[i] = 0;
		cT[i] = 0;
	}
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		if (!IS_ALIVE(pv)) continue;
		if (IS_FREE(pv)    ) ++cF[SCORE(pv) - 1];
		if (IS_TARGETED(pv)) ++cT[SCORE(pv) - 1];
	}
	for (int i = 0; i < nc; ++i) {
		if (cF[i] != cT[i]) THROW(DDS_ERROR_FT_NUMBER, DDS_MSG_FT_NUMBER);
	}
	LEAVE(ph);
	return STATUS(p);
}