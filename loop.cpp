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
//
// Find and divide loops.
// The variable to be divided willbe set to <F>,and newly created variable is <T> with the name NAME(<DD>)+"+".
//
EXPORT(int) DdsDivideLoop(DDS_PROCESSOR ph)
{
	ENTER(ph);
	bool retry = false;
	int cv = VARIABLE_COUNT();
	STACK(cv/2);
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		CLEAR_PROC_FLAG(pv);
		SCORE(pv) = 0;
		if (!IS_ALIVE(pv)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		if (DDS_FLAG_OR(SYS_FLAG(pv), DDS_FLAG_SET | DDS_FLAG_TARGETED | I_BACKTRACK() | DDS_SFLAG_FREE)) {
			SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		}
	}

	auto CHECK_LOOP = [&]() {
		DdsVariable* v_route = nullptr;
		do {
			retry = false;
			v_route = nullptr;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
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
		for (int i = 0; i < cv; ++i) SCORE(VARIABLE(i)) = 0;
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH_F(pv);
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_TARGETED | I_BACKTRACK() | DDS_SFLAG_DIVIDED);
		while ((pv = PEEK()) != nullptr) {
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_TARGETED | I_BACKTRACK() | DDS_SFLAG_DIVIDED);
			if (INDEX(pv) < 0) { POP_F(); }
			else {
				DdsVariable* rv = RHSV(pv, INDEX(pv));
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

		//
		// create 1 more variable and set it to be <T>.
		//
		int re = RHSV_EX();
		RHSV_EX() = 1;
		int e = DdsAddVariableA(p, &pv, NAME(v_divided), USER_FLAG(v_divided), VALUE(v_divided), FUNCTION(v_divided), RHSV_COUNT(v_divided), (DDS_VARIABLE**)RHSVs(v_divided));
		RHSV_EX() = re;
		if (e != 0) THROW(e, "Can not divide a variable (or create a new variable) in DdsDivideLoop()");
		SET_SFLAG_ON(pv       , SYS_FLAG(v_divided) | DDS_FLAG_TARGETED | DDS_SFLAG_DIVIDED | DDS_SFLAG_CHECKED);
		SET_SFLAG_ON(v_divided,                       DDS_SFLAG_FREE    | DDS_SFLAG_DIVIDED | DDS_SFLAG_CHECKED);
		int l = strlen(NAME(pv));
		RHSV(pv, RHSV_COUNT(pv)) = v_divided;
		((char*)NAME(pv))[l] = '+'; // sign of divided and added variable.
	};

	// Find loop => divide it if found.
	while (DdsVariable* v_roue = CHECK_LOOP()) {
		// Loop exists!
		ASSERT(!IS_CHECKED(v_roue));
		// pv is on any loop!
		DIVIDE_LOOP(v_roue);
	}
	LEAVE(ph);
	return STATUS();
}
