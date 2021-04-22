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

#define score_non_div  1
#define score_normal   10
#define score_divide   100

EXPORT(int) DdsDivideLoop(DDS_PROCESSOR ph)
{
	ENTER(ph);
	bool retry = false;
	int cv = VARIABLE_COUNT(p);
	STACK(cv + 1);
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(p, i);
		CLEAR_PROC_FLAG(pv);
		SCORE(pv) = 0;
		if (!IS_ALIVE(pv)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		if (DDS_FLAG_OR(SYS_FLAG(pv), DDS_FLAG_SET | DDS_FLAG_TARGETED | DDS_FLAG_INTEGRATED | DDS_SFLAG_FREE)) {
			SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		}
	}

	auto CHECK_LOOP = [&]() {
		DdsVariable* v_route = nullptr;
		do {
			retry = false;
			v_route = nullptr;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(p, i);
				if (IS_CHECKED(pv)) continue;
				int ir = RHSV_COUNT(pv);
				while (--ir >= 0) {	if (!IS_CHECKED(RHSV(pv, ir))) break; }
				if (ir < 0) {
					SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
					retry = true;
				} else {
					v_route = pv;
				}
			}
		} while (retry);
#ifdef _DEBUG
		if (v_route!=nullptr) printf("CHECK_LOOP(): Loop exists(search loop from %s)!",NAME(v_route));
		else printf("CHECK_LOOP(): No loop found!");
#endif
		return v_route;
	};

	auto DIVIDE_LOOP = [&](DdsVariable *pv) {
		DdsVariable* v_divided     = pv;
		int          score_divided = 0;
		for (int i = 0; i < cv; ++i) SCORE(VARIABLE(p, i)) = 0;
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH_F(pv);
		ENABLE_BACKTRACK();
		while ((pv = PEEK()) != nullptr) {
			DdsVariable* rv = MOVE_BACK(pv);
			if (rv==nullptr) { POP_F(); }
			else {
				if (IS_STACKED(rv)) {
					// rv is stacked ==> rv is on the loop (in STACK)
					int ix = STACK_SIZE();
					DdsVariable* sv;
					do {
						ASSERT(ix > 0);
						sv = STACK_ELEMENT(--ix);
						if (IS_NDIVISIBLE(sv)) SCORE(sv) += score_non_div;
						else if (IS_DIVISIBLE(sv)) SCORE(sv) += score_divide;
						else SCORE(sv) += score_normal;
						if (SCORE(sv) > score_divided) {
							score_divided = SCORE(sv);
							v_divided = sv;
						}
					} while (sv != rv);
				}
				PUSH_F(rv);
			}
		}
		ASSERT(v_divided!=nullptr);
		TRACE_EX((" --variable (%s) divided! ",NAME(v_divided)));
		SET_SFLAG_ON(v_divided, DDS_SFLAG_DIVIDED|DDS_SFLAG_CHECKED);
	};

	// Find loop => divide it if found.
	while (DdsVariable* v_roue = CHECK_LOOP()) {
		// Loop exists!
		ASSERT(!IS_CHECKED(v_roue));
		// pv is on any loop!
		DIVIDE_LOOP(v_roue);
	}
	LEAVE(ph);
	return STATUS(p);
}
