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

EXPORT(int) DdsBuildSequence(DDS_PROCESSOR ph)
{
	ENTER(ph);
	int cv = VARIABLE_COUNT();
	STACK(cv + 1);

	if(I_COUNT()>0) IVs() = (DdsVariable**)MemAlloc(sizeof(DdsVariable*) * I_COUNT());

	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		CLEAR_PROC_FLAG(pv);
		if (!IS_ALIVE(pv)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		NEXT(pv) = nullptr;
		if (!IS_VOLATILE(pv)) {
			// non-volatile <S> or <T> is a constant(never change it's value.)
			if(IS_SFLAG_OR(pv,DDS_FLAG_SET)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		}
		if (IS_INTEGRATED(pv)) {
			SET_SFLAG_ON(pv, DDS_FLAG_VOLATILE);
			SET_SFLAG_ON(RHSV(pv,0), DDS_SFLAG_DERIVATIVE);
		}
	}

	auto ORDER_BLOCK = [&cv, &p](DdsVariable*& v_from, DdsVariable*& v_to,int label, unsigned int f) {
		bool retry = false;
		// Order, variables the block labelledlabel.
		for (int i = 0; i < T_COUNT(); ++i) {
			if (label != SCORE(TV(i))) continue;
			if (v_from == nullptr) {
				v_from = F_PAIRED(i);
				v_to = v_from;
			} else {
				NEXT(v_to) = F_PAIRED(i);
				v_to       = F_PAIRED(i);
			}
			NEXT(v_to) = nullptr;
			SET_SFLAG_ON(v_to,DDS_SFLAG_CHECKED);
		}
		do {
			retry = false;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
				if (SCORE(pv) != label) continue;
				int nr = RHSV_COUNT(pv);
				if (nr <= 0 || IS_CHECKED(pv)) continue;
				int j;
				for (j = 0; j < nr; ++j) if (!IS_CHECKED(RHSV(pv, j))) break;
				if (j < nr) continue;
				// All RHSVs checked! ==> add to list.
				SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED | (f));
				retry = true;
				if (v_from == nullptr) {
					v_from = pv;
					v_to = pv;
				}				else {
					NEXT(v_to) = pv;
					v_to = pv;
				}
				NEXT(v_to) = nullptr;
			}
		} while (retry);
	};

	auto ORDER_NON_BLOCK = [&cv,&p](DdsVariable* &v_from,DdsVariable* &v_to,unsigned int f) {
		bool retry = false;
		//
		// order variables not in any block.
		do {
			retry = false;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
				int nr = RHSV_COUNT(pv);
				if (nr<=0 || IS_CHECKED(pv)) continue;
				int j;
				for (j = 0; j < nr; ++j) if (!IS_CHECKED(RHSV(pv,j))) break;
				if (j < nr) continue;
				// All RHSVs checked! ==> add to list.
				SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED|(f));
				retry = true;
				if (v_from == nullptr) {
					v_from = pv;
					v_to   = pv;
					NEXT(v_to) = nullptr;
				} else {
					NEXT(v_to) = pv;
					v_to = pv;
					NEXT(pv) = nullptr;
				}
			}
		} while (retry);
	};

	//
	// Make order list for variable computed by <S> or non-volatile <T>
	ORDER_NON_BLOCK(V_TOP(), V_END(), DDS_COMPUTED_ONCE);

	// 
	// Back track from <DE> to <V>(including <I>) and set DDS_COMPUTED_EVERY_TIME to the variables on the route.
	for (int i = 0; i < I_COUNT(); ++i) {
		DdsVariable* pv = RHSV(IV(i),0);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED);
		while ((pv = PEEK()) != nullptr) {
			if (IS_VOLATILE(pv)) {
				int ix = STACK_SIZE();
				do {
					SET_SFLAG_ON(STACK_ELEMENT(--ix), DDS_COMPUTED_EVERY_TIME);
				} while (ix>1);
			}
			MOVE_BACK(pv, DDS_FLAG_SET| DDS_SFLAG_FREE);
			if (INDEX(pv) < 0) POP();
			else PUSH(RHSV(pv, INDEX(pv)));
		}
	}

	// 
	// Back track from <T> to <F>  and set DDS_COMPUTED_EVERY_TIME to every variables
	// on the route if the route contains DDS_COMPUTED_EVERY_TIME.
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED);
		while ((pv = PEEK()) != nullptr) {
			if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) {
				int ix = STACK_SIZE();
				do {
					if (IS_SFLAG_OR(STACK_ELEMENT(--ix), DDS_COMPUTED_EVERY_TIME)) break;
					SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_EVERY_TIME);
				} while (ix > 1);
				// Set flag to <F>s & <T>s in the same block.
				int block = SCORE(TV(i));
				for (int j = 0; j < T_COUNT(); ++j) {
					if (SCORE(TV(j)) == block) {
						SET_SFLAG_ON(TV(j), DDS_COMPUTED_EVERY_TIME);
						SET_SFLAG_ON(F_PAIRED(j), DDS_COMPUTED_EVERY_TIME);
					}
				}
			}
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED);
			if (INDEX(pv) < 0) POP();
			else PUSH(RHSV(pv, INDEX(pv)));
		}
		//
		// VOLATILE/DDS_COMPUTED_ANY_TIME processing.
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pv = TV(i);
			if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) continue;
			if (IS_VOLATILE(pv)) {
				SET_SFLAG_ON(pv, DDS_COMPUTED_ANY_TIME);
				int block = SCORE(TV(i));
				for (int j = 0; j < T_COUNT(); ++j) {
					if (SCORE(TV(j)) == block) {
						SET_SFLAG_ON(TV(j), DDS_COMPUTED_ANY_TIME);
						SET_SFLAG_ON(F_PAIRED(j), DDS_COMPUTED_ANY_TIME);
					}
				}
			}
		}
	}
	// 
	// Back track from <T>(DDS_COMPUTED_ANY_TIME) to <F>  and set DDS_COMPUTED_ANY_TIME to every variables
	// on the route if the route contains DDS_COMPUTED_EVERY_TIME/DDS_COMPUTED_ANY_TIME.
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		if (!IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME)) continue;
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED);
		while ((pv = PEEK()) != nullptr) {
			if (pv != TV(i) && IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME | DDS_COMPUTED_ANY_TIME | DDS_FLAG_VOLATILE)) {
				int ix = STACK_SIZE();
				do {
					if (IS_SFLAG_OR(STACK_ELEMENT(--ix), DDS_COMPUTED_ANY_TIME)) break;
					SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME);
				} while (ix > 1);
			}
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED);
			if (INDEX(pv) < 0) POP();
			else PUSH(RHSV(pv, INDEX(pv)));
		}
	}

	// Now continue sequencing(time independent block)!
	int label = -1;
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		if (label == SCORE(pv)) continue;
		label = SCORE(pv);
		if (IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME | DDS_COMPUTED_EVERY_TIME)) continue;
		ORDER_BLOCK(V_TOP(), V_END(), label, DDS_COMPUTED_ONCE);
		ORDER_NON_BLOCK(V_TOP(), V_END(), DDS_COMPUTED_ONCE);
	}
	//
	// Time dependent part!
	// EVERY TIME
	label = -1;
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		if (label == SCORE(pv)) continue;
		label = SCORE(pv);
		if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) {
			ORDER_BLOCK(V_TOP(), V_END(), label, DDS_COMPUTED_EVERY_TIME);
			ORDER_NON_BLOCK(V_TOP(), V_END(), DDS_COMPUTED_EVERY_TIME);
		}
	}
	// ANY TIME
	label = -1;
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		if (label == SCORE(pv)) continue;
		label = SCORE(pv);
		if (IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME)) {
			ORDER_BLOCK(V_TOP(), V_END(), label, DDS_COMPUTED_ANY_TIME);
			ORDER_NON_BLOCK(V_TOP(), V_END(), DDS_COMPUTED_ANY_TIME);
		}
	}

#ifdef _DEBUG
	DdsVariable* pv = V_TOP();
	int ic = 0;
	printf("SEQUENCE: ");
		do {
			printf(" [%d]%s ", ic++, NAME(pv));
			pv = NEXT(pv);
		} while (pv != nullptr);
#endif

	LEAVE(ph);
	return STATUS();
}
