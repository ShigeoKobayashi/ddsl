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

//
// Sets DDS_SFLAG_DERIVATIVE if it is the RHSV of <I>.
// and defines computing sequence of each variable as follows:
// 
//   from V_TOP_ONCET() to V_END_ONCET() is a linked list of variables computed at ONCE at the beginning.
//        v = V_TOP_ONCET() is comouted first, then v = NEXTv) ,..., to V_END_ONCET()
//   then V_TOP_ANYT()  to V_END_ANYT(), for the variables computed at every integration step.
//   finally,
//        V_TOP_EVERYT() to V_END_EVERYT() at anytime(is not computed at every integration steps,but is time dependent).
//
EXPORT(int) DdsBuildSequence(DDS_PROCESSOR ph)
{
	ENTER(ph);
	int cv = VARIABLE_COUNT();
	STACK(cv/2);

	V_TOP_ONCET()  = nullptr;
	V_END_ONCET()  = nullptr;
	V_TOP_ANYT()   = nullptr;
	V_END_ANYT()   = nullptr;
	V_TOP_EVERYT() = nullptr;
	V_END_EVERYT() = nullptr;

	if(I_COUNT()>0) IVs() = (DdsVariable**)MemAlloc(p,sizeof(DdsVariable*) * I_COUNT());
	int n_volatile = 0; // <V> counter
	int block      = 0; // block label
	int ni         = 0; // <I> counter
	bool retry     = false;

	auto REGISTER = [&cv, &p,&retry](DdsVariable*& v_from, DdsVariable*& v_to, unsigned int f) {
		int j, nr;
		do { retry = false;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
				if (IS_CHECKED(pv)            ) continue;
				if (!IS_SFLAG_OR(pv,f)        ) continue;
				if ((nr = RHSV_COUNT(pv)) <= 0) continue;
				for (j = 0; j < nr; ++j) if (!IS_CHECKED(RHSV(pv, j))) break;
				if (j < nr) continue;
				SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED|(f));
				retry = true;
				if (v_from == nullptr) {
					v_from = pv; v_to = pv;	NEXT(v_to) = nullptr;
				} else {
					NEXT(v_to) = pv; v_to = pv; NEXT(pv) = nullptr;
				}
			}
		} while (retry);
	};

	auto REGISTER_BLOCK = [&cv, &p,&retry,&REGISTER](DdsVariable*& v_from, DdsVariable*& v_to,int block, unsigned int f) {
		for (int i = 0; i < T_COUNT(); ++i) {
			if (block != SCORE(TV(i))) continue;
			// register <F>s in the specified block first.
			if (v_from == nullptr) {
				v_from = F_PAIRED(i);
				v_to = v_from;
			} else {
				NEXT(v_to) = F_PAIRED(i);
				v_to       = F_PAIRED(i);
			}
			SET_SFLAG_ON(F_PAIRED(i), DDS_SFLAG_CHECKED|(f));
			NEXT(v_to) = nullptr;
			SET_SFLAG_ON(v_to,DDS_SFLAG_CHECKED);
		}
		int j, nr;
		do {
			retry = false;
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
				if (IS_CHECKED(pv)            ) continue;
				if (SCORE(pv)!=block          ) continue;
				if (!IS_SFLAG_OR(pv, f)       ) continue;
				if ((nr = RHSV_COUNT(pv)) <= 0) continue;
				for (j = 0; j < nr; ++j) {
					if (!IS_CHECKED(RHSV(pv, j))) break;
				}
				if (j < nr) continue;
				SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED | f);
				retry = true;
				if (v_from == nullptr) {
					v_from = pv; v_to = pv;	NEXT(v_to) = nullptr;
				} else {
					NEXT(v_to) = pv; v_to = pv; NEXT(pv) = nullptr;
				}
			}
		} while (retry);
		// finally register <T>(because <T> is already CHECKED before called!)
		// registering order is that of TV() order.
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pv = TV(i);
			if (SCORE(pv) != block) continue;
			if (v_from == nullptr) {
				v_from = pv; v_to = pv;	NEXT(v_to) = nullptr;
			}
			else {
				NEXT(v_to) = pv; v_to = pv; NEXT(pv) = nullptr;
			}
		}
	};

	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		CLEAR_PROC_FLAG(pv);
		NEXT(pv) = nullptr;
		if (!IS_ALIVE(pv)) { SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED); continue; }
		if (IS_INTEGRATED(pv)) {
			// <I>
			SET_SFLAG_ON(pv, DDS_FLAG_VOLATILE| DDS_COMPUTED_EVERY_TIME);       // Set <I> be volatile.
			SET_SFLAG_ON(RHSV(pv, 0), DDS_SFLAG_DERIVATIVE| DDS_FLAG_VOLATILE); // <DR> also volatile.
			IV(ni++) = pv;
		}
		if (IS_VOLATILE(pv)) {
			++n_volatile;
			SET_SFLAG_ON(pv, DDS_COMPUTED_ANY_TIME); // Set <V> is <AT> at least.
		} else {
			// Non-volatile <S> and <T>
			if (IS_SFLAG_OR(pv, DDS_FLAG_SET|DDS_FLAG_TARGETED)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED|DDS_COMPUTED_ONCE);
		}
	}
	ASSERT(ni == I_COUNT());

	//
	// SET computation flags
	//
	if (n_volatile > 0) {
		// Back track  <R> to <V> and set DDS_COMPUTED_ANY_TIME to variables on the route.
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(i);
			if (!IS_REQUIRED(pv)) continue;
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			block = SCORE(pv);
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			while ((pv = PEEK()) != nullptr) {
				if (IS_SFLAG_OR(pv, DDS_FLAG_VOLATILE)) {
					SET_SFLAG_ON(pv, DDS_COMPUTED_ANY_TIME);
					int ix = STACK_SIZE();
					while (--ix > 0) {
						SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME);
					}
					if (block >= 0) {
						for (int j = 0; j < cv; ++j) {
							if (SCORE(VARIABLE(j)) == block) SET_SFLAG_ON(VARIABLE(j), DDS_COMPUTED_ANY_TIME);
						}
					}
				}
				MOVE_BACK(pv, DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else PUSH(RHSV(pv, INDEX(pv)));
			}
		}
		// 
		// Back track non-checked <T> to <V> and set DDS_COMPUTED_ANY_TIME to variables on the route.
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pv = TV(i);
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			block = SCORE(pv);
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			while ((pv = PEEK()) != nullptr) {
				if (IS_SFLAG_OR(pv, DDS_FLAG_VOLATILE)) {
					SET_SFLAG_ON(pv, DDS_COMPUTED_ANY_TIME);
					int ix = STACK_SIZE();
					while (--ix > 0) {
						SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME);
					}
					for (int j = 0; j < cv; ++j) {
						if (SCORE(VARIABLE(j)) == block) SET_SFLAG_ON(VARIABLE(j), DDS_COMPUTED_ANY_TIME);
					}
				}
				MOVE_BACK(pv, DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else PUSH(RHSV(pv, INDEX(pv)));
			}
		}
		// 
		// Back track from <DR> to <V>(including <I>) and set DDS_COMPUTED_EVERY_TIME to the variables on the route.
		for (int i = 0; i < I_COUNT(); ++i) {
			DdsVariable* pv = RHSV(IV(i), 0);
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv); // pv==<DR>
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			while ((pv = PEEK()) != nullptr) {
				if (IS_VOLATILE(pv)) {
					SET_SFLAG_ON(pv, DDS_COMPUTED_EVERY_TIME);
					// get from <I> back to <V>
					int ix = STACK_SIZE();
					while (--ix > 0) {
						pv = STACK_ELEMENT(ix);
						if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) continue;
						SET_SFLAG_ON(pv, DDS_COMPUTED_EVERY_TIME);
						if ((block=SCORE(pv)) > 0) {
							// pv is on <F>-<T> route.
							for (int j = 0; j < cv; ++j) {
								if (SCORE(VARIABLE(j)) == block) {
									SET_SFLAG_ON(VARIABLE(j), DDS_COMPUTED_EVERY_TIME);
								}
							}
						}
					}
				} else if (IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME)) {
					// pv is on the <T>-<V> route
					block = SCORE(pv);
					if (block > 0) {
						for (int j = 0; j < cv; ++j) {
							if (SCORE(VARIABLE(j)) == block) SET_SFLAG_ON(VARIABLE(j), DDS_COMPUTED_EVERY_TIME);
						}
					}
				}
				MOVE_BACK(pv, DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else               PUSH(RHSV(pv, INDEX(pv)));
			}
		}
		// 
		// Back track from EVERY_TIME <T> to <V> and set DDS_COMPUTED_EVERY_TIME to variables on the route.
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pv = TV(i);
			if (!IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) continue;
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			while ((pv = PEEK()) != nullptr) {
				if (IS_SFLAG_OR(pv, DDS_FLAG_VOLATILE)) {
					int ix = STACK_SIZE();
					while (--ix > 0) {
						if (IS_SFLAG_OR(STACK_ELEMENT(ix), DDS_COMPUTED_EVERY_TIME)) break;
						SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_EVERY_TIME);
					}
				}
				MOVE_BACK(pv, DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else PUSH(RHSV(pv, INDEX(pv)));
			}
		}
		// 
		// Back track non-checked <T> to <V>(or any <ET> variable) and set DDS_COMPUTED_ANY_TIME to variables on the route.
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pv = TV(i);
			if (IS_SFLAG_OR(pv,DDS_COMPUTED_EVERY_TIME)) continue;
			STACK_CLEAR();
			PUSH(nullptr);
			PUSH(pv);
			block = SCORE(pv);
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			while ((pv = PEEK()) != nullptr) {
				if (IS_SFLAG_OR(pv, DDS_FLAG_VOLATILE|DDS_COMPUTED_EVERY_TIME)) {
					SET_SFLAG_ON(pv, DDS_COMPUTED_ANY_TIME);
					int ix = STACK_SIZE();
					while (--ix > 0) {
						SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME);
					}
					for (int j = 0; j < cv; ++j) {
						if (SCORE(VARIABLE(j)) == block) SET_SFLAG_ON(VARIABLE(j),DDS_COMPUTED_ANY_TIME);
					}
				}
				MOVE_BACK(pv, DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else PUSH(RHSV(pv, INDEX(pv)));
			}
		}
		// Back track from non-checked variable to any <ET>/<AT> variable, and set DDS_COMPUTED_ANY_TIME to variables on the route.
		do {
			retry = false;
			ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | I_BACKTRACK());
			for (int i = 0; i < cv; ++i) {
				DdsVariable* pv = VARIABLE(i);
				if (!IS_ALIVE(pv)) continue;
				if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) continue;
				if (IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME)) continue;
				STACK_CLEAR();
				PUSH(nullptr);
				PUSH(pv);
				while ((pv = PEEK()) != nullptr) {
					if (IS_SFLAG_OR(pv, DDS_FLAG_VOLATILE | DDS_COMPUTED_EVERY_TIME | DDS_COMPUTED_ANY_TIME)) {
						int ix = STACK_SIZE() - 1;
						retry = true;
						while (--ix > 0) {
							if (IS_SFLAG_OR(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME)) break;
							SET_SFLAG_ON(STACK_ELEMENT(ix), DDS_COMPUTED_ANY_TIME);
							block = SCORE(STACK_ELEMENT(ix));
							if (block > 0) {
								for (int jj = 0; jj < T_COUNT(); ++jj) {
									if (SCORE(TV(jj)) == block) {
										SET_SFLAG_ON(TV(jj), DDS_COMPUTED_ANY_TIME);
										SET_SFLAG_ON(F_PAIRED(jj), DDS_COMPUTED_ANY_TIME);
									}
								}
							}
						}
					}
					MOVE_BACK(pv, DDS_FLAG_SET);
					if (INDEX(pv) < 0) POP();
					else PUSH(RHSV(pv, INDEX(pv)));
				}
			}
		} while (retry);
	}
#ifdef _DEBUG
	DdsDbgPrintF(stdout, "== DdsBuildSequence(p): after flagging:", p);
#endif
	// ======== ONCE ========
	// First register variables computed directly or indirectly by non-volatile <S> or <T>
	REGISTER(V_TOP_ONCET(), V_END_ONCET(), DDS_SFLAG_ALIVE | DDS_COMPUTED_ONCE);
	// then block
	for (int i = 0; i < T_COUNT(); ++i) {
		if (IS_SFLAG_OR(F_PAIRED(i), DDS_COMPUTED_ANY_TIME | DDS_COMPUTED_EVERY_TIME)) continue;
		if (IS_CHECKED(F_PAIRED(i))                                                  ) continue;
		block = SCORE(F_PAIRED(i));
		for (int j = i; j < T_COUNT(); ++j) {
			if (SCORE(F_PAIRED(i)) == block) SET_SFLAG_ON(F_PAIRED(i), DDS_COMPUTED_ONCE|DDS_SFLAG_CHECKED);
		}
		REGISTER_BLOCK(V_TOP_ONCET(), V_END_ONCET(), block, DDS_SFLAG_ALIVE|DDS_COMPUTED_ONCE);
		REGISTER(V_TOP_ONCET(), V_END_ONCET(), DDS_SFLAG_ALIVE | DDS_COMPUTED_ONCE);
	}

	if (n_volatile > 0) {
		// ======== EVERY-TIME =========
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(i);
			if (IS_SFLAG_OR(pv, DDS_FLAG_SET | DDS_FLAG_TARGETED | I_BACKTRACK())) {
				if (IS_SFLAG_OR(pv, DDS_COMPUTED_EVERY_TIME)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
			}
		}
		REGISTER(V_TOP_EVERYT(), V_END_EVERYT(), DDS_COMPUTED_EVERY_TIME);
		// then block
		for (int i = 0; i < T_COUNT(); ++i) {
			if (!IS_SFLAG_OR(F_PAIRED(i), DDS_COMPUTED_EVERY_TIME)) continue;
			if ( IS_CHECKED(F_PAIRED(i))                          ) continue;
			block = SCORE(F_PAIRED(i));
			for (int j = i; j < T_COUNT(); ++j) {
				if (SCORE(F_PAIRED(i)) == block) SET_SFLAG_ON(F_PAIRED(i), DDS_SFLAG_CHECKED);
			}
			REGISTER_BLOCK(V_TOP_EVERYT(), V_END_EVERYT(), block, DDS_COMPUTED_EVERY_TIME);
			REGISTER(V_TOP_EVERYT(), V_END_EVERYT(), DDS_COMPUTED_EVERY_TIME);
		}

		// ======== ANY-TIME =========
		for (int i = 0; i < cv; ++i) {
			DdsVariable* pv = VARIABLE(i);
			if (IS_SFLAG_OR(pv, DDS_FLAG_SET | DDS_FLAG_TARGETED | I_BACKTRACK())) {
				if (IS_SFLAG_OR(pv, DDS_COMPUTED_ANY_TIME)) SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
			}
		}
		REGISTER(V_TOP_ANYT(), V_END_ANYT(), DDS_COMPUTED_ANY_TIME);
		// then block
		for (int i = 0; i < T_COUNT(); ++i) {
			if (!IS_SFLAG_OR(F_PAIRED(i), DDS_COMPUTED_ANY_TIME)) continue;
			if ( IS_CHECKED(F_PAIRED(i))                        ) continue;
			block = SCORE(F_PAIRED(i));
			for (int j = i; j < T_COUNT(); ++j) {
				if (SCORE(F_PAIRED(i)) == block) SET_SFLAG_ON(F_PAIRED(i), DDS_SFLAG_CHECKED);
			}
			REGISTER_BLOCK(V_TOP_ANYT(), V_END_ANYT(), block, DDS_COMPUTED_ANY_TIME);
			REGISTER(V_TOP_ANYT(), V_END_ANYT(), DDS_SFLAG_ALIVE | DDS_COMPUTED_ANY_TIME);
		}
	}
	//
	// 
#ifdef _DEBUG
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		if (IS_ALIVE(pv)) {
			if (IS_SFLAG_OR(pv, DDS_FLAG_SET | I_BACKTRACK())) continue;
			SET_SFLAG_OFF(pv, DDS_SFLAG_CHECKED);
		}
	}
	DdsVariable* pv = V_TOP_ONCET();
	int ic = 0;
	printf("\nONCE SEQUENCE      : ");
	do {
		if (pv == nullptr) break;
		printf(" %d)%s ", ic++, NAME(pv));
		ASSERT(!IS_CHECKED(pv));
		SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		pv = NEXT(pv);
	} while (pv != nullptr);
	pv = V_TOP_EVERYT();
	ic = 0;
	printf("\nEVERY_TIME SEQUENCE: ");
	do {
		if (pv == nullptr) break;
		printf(" %d)%s ", ic++, NAME(pv));
		ASSERT(!IS_CHECKED(pv));
		SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		if (pv == V_END_EVERYT()) break;
		pv = NEXT(pv);
	} while (pv != nullptr);
	pv = V_TOP_ANYT();
	ic = 0;
	printf("\nANY_TIME SEQUENCE  : ");
	do {
		if (pv == nullptr) break;
		printf(" %d)%s ", ic++, NAME(pv));
		ASSERT(!IS_CHECKED(pv));
		SET_SFLAG_ON(pv, DDS_SFLAG_CHECKED);
		if (pv == V_END_ANYT()) break;
		pv = NEXT(pv);
	} while (pv != nullptr);
	printf("\n");
	for (int i = 0; i < cv; ++i) {
		ASSERT(IS_CHECKED(VARIABLE(i)));
	}
#endif

	LEAVE(ph);
	return STATUS();
}
