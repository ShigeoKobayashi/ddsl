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

static void HeapSort(DdsProcessor* p);

//
// Performs block-triangular decomposition.
//  On exit:
//      TV(i) and F_PIRED(i) are sorted in computation(block) order.
//      INDEX(<T>),INDEX(<F>) point the array index i of TV(i) and F_PAIRED(i).
//      SCORE(<T>),SCORE(<F>) have block number(=block index+1) that belong to.
//      SCORE(<X>),<X> is any variable other than <F> or <T>,is -1 if <X> is not on any
//                 <F>-<T> block. 
//                 The value other than -1 means the variable is on the block of <T>(SCORE(<X>)==SCORE(<T>)).
//      B_COUNT() ... total number of blocks.
//      B_SIZE(i) ... the size of i-th block.
//
EXPORT(int) DdsCheckRouteFT(DDS_PROCESSOR ph)
{
	ENTER(ph);
	int cv = VARIABLE_COUNT();
	STACK(cv + 1);

	// count <T>s
	T_COUNT() = 0;
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		if (IS_ALIVE(pv) && IS_TARGETED(pv)  ) ++T_COUNT(); // count <T>
		if (IS_ALIVE(pv) && IS_INTEGRATED(pv)) ++I_COUNT(); // count <I>
	}
	TRACE_EX(("DdsCheckRouteFT(): <T>s=%d",T_COUNT()));
	if (T_COUNT() <= 0) return 0; // nothing to do here!

	//
	// allocate <F> & <T> related memories.
	TVs()          = (DdsVariable**)MemAlloc(sizeof(DdsVariable*) * T_COUNT());
	F_COUNTs()     = (int*)MemAlloc(sizeof(int) * T_COUNT());
    F_MAX_COUNTs() = (int*)MemAlloc(sizeof(int) * T_COUNT());
    FT_MATRIX()    = (DdsVariable***)MemAlloc(sizeof(DdsVariable**) * T_COUNT());
	for(int i=0;i<T_COUNT();++i)  Fs_CONNECTED(i) = (DdsVariable**)MemAlloc(sizeof(DdsVariable*) * 2);

	int ix = 0;
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		SCORE(pv) = -1;
		NEXT(pv) = nullptr;
		if (IS_ALIVE(pv) && IS_TARGETED(pv)) {
			SCORE(pv) = ix; // Route ID!
			TV(ix++) = pv;
		}
	}

	auto ADD_F = [&p](int i, DdsVariable* f) {
		if (F_COUNT(i) >= F_MAX_COUNT(i)) {
			F_MAX_COUNT(i) += F_MAX_COUNT(i) / 5 + 2;
			Fs_CONNECTED(i) = (DdsVariable**)MemRealloc(Fs_CONNECTED(i),sizeof(DdsVariable*) * F_MAX_COUNT(i));
		}
		Fs_CONNECTED(i)[F_COUNT(i)++] = f;
	};

	auto DETACH_CROSS_NODE = [&](DdsVariable* pr) {
		DdsVariable* pv = NEXT(pr);
		NEXT(pr) = PEEK(); // change route!
		do {
			DdsVariable* pn = NEXT(pv);
			ASSERT(!IS_STACKED(pv));
			SCORE(pv) = -1;
			NEXT(pv) = nullptr;
			pv = pn;
		} while (!IS_STACKED(pv));
		// re-route!
		do {
			pv = PEEK();
			POP_F();
		} while (!IS_TARGETED(pv));
		pv = PEEK();
		ASSERT(INDEX(pv) >= 0);
		PUSH_F(RHSV(pv,INDEX(pv)));
	};

	auto GET_ROUTE_ID = [&](bool cancel) {
		int ix = STACK_SIZE();
		while (--ix > 0) if (IS_TARGETED(STACK_ELEMENT(ix))) break;
		ASSERT(ix > 0);
		DdsVariable* pv = STACK_ELEMENT(ix);
		ix = SCORE(pv);
		if (cancel && (NEXT(pv) != nullptr)) {
			// pv(==<T>) is previously paired another <F> == cancel it!
			DdsVariable* pn = pv;
			do {
				SCORE(pv) = -1;
				pn = NEXT(pv);
				NEXT(pv) = nullptr;
				pv = pn;
			} while (!IS_TARGETED(pv));
		}
		return ix; // route ID.
	};


	auto REACHED_F = [&](DdsVariable *pf) {
		TRACE(("Reached to %s <F>.\n", NAME(pf)));
		ASSERT(NEXT(pf) == nullptr);
		do {
			int id = GET_ROUTE_ID(true);
			SCORE(pf) = id; // rooute ID
			DdsVariable* pv = pf;
			do {
				DdsVariable *pn = PEEK();
				NEXT(pv) = pn;
				pv = pn;
				SCORE(pv) = id;
				POP_F();
			} while (!IS_TARGETED(pv));
			NEXT(pv) = pf; // NEXT(<T>) = paired <F>
			if ((pv=PEEK()) != nullptr) {
				do {
					ASSERT(INDEX(pv)>=0);
					pv = RHSV(pv, INDEX(pv));
					PUSH(pv);
				} while (!IS_FREE(pv));
				pf = pv;
				POP_F();
			}
		} while (PEEK() != nullptr);
	};

	auto REACHED_ROUTE = [&](DdsVariable* pr) {
		TRACE(("%s is on previously defined route(cross node).\n",NAME(pr)));
		ASSERT(NEXT(pr) != nullptr);
		DdsVariable* pv = PEEK();
		// go to starting <T> which is previously paired to <F>.
		pv = NEXT(pr);
		while (!IS_TARGETED(pv)) pv = NEXT(pv);
		// push back from <T> to the variable before the cross node (cross node is not stacked!)
		do {
			PUSH_F(pv);
		} while ((pv = RHSV(pv, INDEX(pv))) != pr);
	};

	//
	// Confirm/make 1 to 1 correspondence.
	ENABLE_BACKTRACK( DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED );
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);

		TRACE(("\nFrom <T> %s ",NAME(pv)));

		STACK_CLEAR();
		PUSH(nullptr);
		PUSH_F(pv);
		bool ok = false;
		while ((pv = PEEK()) != nullptr) {
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_INTEGRATED | DDS_SFLAG_STACKED);
			if(INDEX(pv) < 0) { POP_F(); continue; }
			DdsVariable *rv = RHSV(pv, INDEX(pv));
			TRACE(("%s ", NAME(rv)));
			if (NEXT(rv) != nullptr) { 
				// reached to previously defined route.
				int id = GET_ROUTE_ID(false);
				if (id == SCORE(rv)) DETACH_CROSS_NODE(rv); // reached to current route again ==> detach redundant nodes.
				else                 REACHED_ROUTE(rv);     // reached to different route.
			} else if (IS_FREE(rv)) {
				REACHED_F(rv);      // reached to <F>
				ok = true;
			} else {
				PUSH_F(rv);
			}
		}
		if (!ok) THROW(DDS_ERROR_FT_ROUTE, DDS_MSG_FT_ROUTE);
	}
	// register paired <T>-<F>
	for (int i = 0; i < T_COUNT(); ++i) {
		ADD_F(i, NEXT(TV(i)));
	}
	// register non-paired <T>-<F>
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		for (int j = 0; j < T_COUNT(); ++j) {
			SET_SFLAG_OFF(F_CONNECTED(j, 0), DDS_SFLAG_CHECKED);
		}
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		SET_SFLAG_ON(F_CONNECTED(i, 0), DDS_SFLAG_CHECKED);
		while ((pv = PEEK()) != nullptr) {
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED);
			if (INDEX(pv) < 0) { POP_F(); continue; }
			DdsVariable* rv = RHSV(pv, INDEX(pv));
			if (IS_FREE(rv) && !IS_CHECKED(rv)) {
				ADD_F(i, rv);
				SET_SFLAG_ON(rv, DDS_SFLAG_CHECKED);
			} else {
				PUSH(rv);
			}
		}
	}

	//
	// then perform triangular-decomposition
	auto MERGE = [](DdsVariable* v1, DdsVariable* v2) {
		int s1 = SCORE(v1);
		int s2 = SCORE(v2);
		int s = s1;
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

	// lopp of <Fs> indicates the block decmposed.
	auto NEXT_T = [&p](DdsVariable* pt) {
		if (SCORE(pt) <= 0) return (DdsVariable*)nullptr;
		int ix = INDEX(pt);
		DdsVariable* pf = F_CONNECTED(ix, --SCORE(pt));
		return TV(INDEX(pf));
	};

	//
	// NEXT(<T>) points PAIRed <F>
	// SCORE(<T>) is a counter of connected <F>s
	// INDEX(<T>) & INDEX(<F>) are the same and point i of F_CONNECTED(i,...)
	//
	// NEXT(<F>) is a circular link of the block decomposed.
	// SCORE(<F>) is the block ID
	// 
	for (int i = 0; i < T_COUNT(); ++i) {
		// <F> setting.
		NEXT(F_PAIRED(i))  = F_PAIRED(i); // self loop
		SCORE(F_PAIRED(i)) = i;
		INDEX(F_PAIRED(i)) = i;
		// <T> setting
		NEXT(TV(i))  = F_PAIRED(i);
		INDEX(TV(i)) = i;
	}
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pt = TV(i);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH_F(pt);
		for (int j = 0; j < T_COUNT(); ++j) SCORE(TV(j)) = F_COUNT(j);
		while ((pt = PEEK()) != nullptr) {
			DdsVariable* nt = NEXT_T(pt);
			if (nt == nullptr) {POP_F();}
			else {
				if (IS_STACKED(nt)) {
					int it = STACK_SIZE();
					for (int jj = it - 1; jj > 0; --jj) {
						DdsVariable* sp = STACK_ELEMENT(jj);
						if (sp == nt) break;
						MERGE(F_PAIRED(INDEX(pt)), F_PAIRED(INDEX(nt)));
					}
				}
				PUSH_F(nt);
			}
		}
	}
	// NEXT(<F>) constructs block & SCORE(<F>) is a block ID.
	for (int i = 0; i < T_COUNT(); ++i) {
		// Copy ID to <T>
		SCORE(TV(i)) = SCORE(F_PAIRED(i));
		// set NEXT(<T>) be circular list as NEXT(<A>)
		NEXT(TV(i)) = TV(INDEX(NEXT(F_PAIRED(i))));
		// reset INDEX
		INDEX(TV(i)) = i;
		INDEX(F_PAIRED(i)) = i;
	}
	//
	//  renumber block ID so that it is the order of computation sequence.
	bool retry = false;
	do {
		retry = false;
		for (int i = 0; i < T_COUNT(); ++i) {
			DdsVariable* pt = TV(i);
			int nf = F_COUNT(i);
			int id1 = SCORE(pt);
			for (int j = 1; j < nf; ++j) {
				DdsVariable* pf = F_CONNECTED(i,j);
				if (SCORE(pf) > id1) {
					// exchange id's
					int id2 = SCORE(pf);
					retry = true;
					for (int k = 0; k < T_COUNT(); ++k) {
						if     (SCORE(TV(k)) == id1) { SCORE(TV(k)) = id2; SCORE(F_PAIRED(k)) = id2;}
						else if(SCORE(TV(k)) == id2) { SCORE(TV(k)) = id1; SCORE(F_PAIRED(k)) = id1; }
					}
					break;
				}
			}
		}
	} while (retry);

	HeapSort(p);

	int nb = 0; // number of blocks
	int cb = -1;// current block id
	for (int i = 0; i < T_COUNT(); ++i) B_SIZE(i) = 0;
	for (int i = 0; i < T_COUNT(); ++i) {
		INDEX(TV(i))       = i;
		INDEX(F_PAIRED(i)) = i;
		if (SCORE(TV(i)) != cb) {
			cb = SCORE(TV(i));
			SCORE(TV(i)) = ++nb;
			B_SIZE(nb-1) = 1;
		} else {
			SCORE(TV(i)) = nb;
			B_SIZE(nb - 1)++;
		}
		SCORE(F_PAIRED(i)) = SCORE(TV(i));
	}
	B_COUNT() = nb;

	//
	// Put SCORE(<T>) to every variables on the route.
	for (int i = 0; i < cv; ++i) {
		DdsVariable* pv = VARIABLE(i);
		if (IS_ALIVE(pv) && IS_SFLAG_OR(pv, DDS_FLAG_TARGETED | DDS_SFLAG_FREE)) continue;
		SCORE(pv) = -1;
	}
	for (int i = 0; i < T_COUNT(); ++i) {
		DdsVariable* pv = TV(i);
		ENABLE_BACKTRACK(DDS_FLAG_SET | DDS_SFLAG_FREE | DDS_FLAG_INTEGRATED);
		STACK_CLEAR();
		PUSH(nullptr);
		PUSH(pv);
		int label = SCORE(pv);
		SET_SFLAG_ON(F_CONNECTED(i, 0), DDS_SFLAG_CHECKED);
		while ((pv = PEEK()) != nullptr) {
			MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED);
			if (INDEX(pv) < 0) { POP_F(); continue; }
			DdsVariable* rv = RHSV(pv, INDEX(pv));
			if (IS_FREE(rv) && label == SCORE(rv)) {
				int ix = STACK_SIZE();
				while (--ix > 1) SCORE(STACK_ELEMENT(ix)) = label;
			} else {
				PUSH(rv);
			}
		}
	}

#ifdef _DEBUG
	printf("BLOCKS:");
	for (int i = 0; i < B_COUNT(); ++i) printf(" %d", B_SIZE(i));
	printf("\n");
	for (int i = 0; i < T_COUNT(); ++i) {
		printf("%d) %d: %s <= ", i, SCORE(TV(i)), NAME(TV(i)));
		for (int j = 0; j < F_COUNT(i); ++j) {
			printf(" %s(%d)", NAME(F_CONNECTED(i, j)), SCORE(F_CONNECTED(i, j)));
		}
		printf("\n");
	}
#endif

	LEAVE(ph);
	return STATUS();
}

static void HeapSort(DdsProcessor *p)
{
	int n = T_COUNT();
	int leaf, root;
	auto COMP = [&p](int i, int j) {
		if (SCORE(TV(i)) > SCORE(TV(j))) return  1;
		if (SCORE(TV(i)) < SCORE(TV(j))) return -1;
		return 0;
	};
	auto SWAP = [&p](int i, int j) {
		DdsVariable ** f = Fs_CONNECTED(i);
		Fs_CONNECTED(i) = Fs_CONNECTED(j);
		Fs_CONNECTED(j) = f;
		DdsVariable* t = TV(i);
		TV(i) = TV(j);
		TV(j) = t;
		int nf = F_COUNT(i);
		F_COUNT(i) = F_COUNT(j);
		F_COUNT(j) = nf;
	};

	// First: construct heap tree of all elements in a(a[0]-a[n-1]) => the root node is a[0] which is the biggest one. 
	leaf = n - 1;
	root = leaf / 2;
	while (root >= 0) {
		int i;
		int r = root--;
		i = r * 2 + 1; // Left son.
		while (i <= leaf) {
			if (i < leaf && COMP(i + 1, i)>0) ++i;  // bigger son will be compared with parent (r).
			if (COMP(r, i) >= 0) break;
			// parent node r is smaller = swap with selected son (i).
			SWAP(r, i);
			r = i; i = r * 2 + 1; // go down to farther sons.
		}
	}

	// Now,swap top(biggest) element and last element of the tree.
	// Shrink the tree(array) and reconstruct the tree.
	// leaf is the index of the last element of the tree. 
	while (leaf > 0) {
		int i;
		int r = 0;
		SWAP(0, leaf--); // after swapping,the tree size is reduced by 1 (leaf--).
		i = r * 2 + 1; // Left son.
		while (i <= leaf) {
			if (i < leaf && COMP(i + 1, i)>0) i++; // Right son is bigger.
			if (COMP(r, i) >= 0) break;
			SWAP(r, i);
			r = i; i = r * 2 + 1;
		}
	}
}
