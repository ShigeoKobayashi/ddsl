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

#define EULER(dt) {for (int i = 0; i < I_COUNT(); ++i) {VALUE(IV(i)) = VALUE(RHSV(IV(i), 0)) * dt;}}

//
// Runge-Kutta integrator.
//
void RK(DdsProcessor* p, double dt)
{
	if (I_COUNT() <= 0) return;
	if (VtoDRs() == nullptr) {
		VtoDRs() = (DdsVariable***)MemAlloc(sizeof(DdsVariable**)*I_COUNT());
		int cv = VARIABLE_COUNT();
		STACK(10);
		for (int i = 0; i < I_COUNT(); ++i) {
			DdsVariable* dr = RHSV(IV(i), 0);
			ENABLE_BACKTRACK(DDS_FLAG_SET|DDS_FLAG_TARGETED|DDS_SFLAG_FREE|DDS_FLAG_INTEGRATED);
			STACK_CLEAR();
			PUSH(dr);
			DdsVariable* pv;
			while ((pv=PEEK()) != dr) {
				if (pv == IV(i)) {POP(); break;}
				MOVE_BACK(pv, DDS_FLAG_SET | DDS_FLAG_TARGETED | DDS_SFLAG_FREE);
				if (INDEX(pv) < 0) POP();
				else PUSH(RHSV(pv, INDEX(pv)));
			}
			VtoDR(i) = (DdsVariable**)MemAlloc(sizeof(DdsVariable*)*STACK_SIZE());
			for (int j = 0; j < STACK_SIZE(); ++j) {
				VtoDR(i)[j] = PEEK();
				POP();
			}
		}
	}

	auto COMPUTE_DR = [&](int i) {
		DdsVariable** pv = VtoDR(i);
		pv--;
		do {
			pv++;
			VALUE(*pv) = FUNCTION(*pv)(p, *pv);
		} while (!IS_DERIVATIVE(*pv));
	};
	//
	// Following conditions are ignored because less value and more time consuming.
	//  1. re-computing any block even if any variable on <F>-<T> route is on <I>-<DR> route.
	//  2. effect on <V> variable on upstream of <DR>.
	//
	for (int i = 0; i < I_COUNT(); ++i) {
		double k1 = VALUE(RHSV(IV(i), 0));
		double IV = VALUE(IV(i));
		VALUE(IV(i)) = IV + k1 * dt / 2.0;
		COMPUTE_DR(i);
		double k2 = VALUE(RHSV(IV(i), 0));
		VALUE(IV(i)) = IV+ k2 * dt / 2.0;
		COMPUTE_DR(i);
		double k3 = VALUE(RHSV(IV(i), 0));
		VALUE(IV(i)) = IV + k3 * dt;
		COMPUTE_DR(i);
		double k4 = VALUE(RHSV(IV(i), 0));
		VALUE(IV(i)) = IV + dt*(k1+2.0*(k2+k3)+k4)/6.0;
	}
}