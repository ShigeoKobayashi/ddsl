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

#define K1s      ((p)->R_K1)
#define K2s      ((p)->R_K2)
#define K3s      ((p)->R_K3)
#define K4s      ((p)->R_K4)
#define K1(i)    (K1s[i])
#define K2(i)    (K2s[i])
#define K3(i)    (K3s[i])
#define K4(i)    (K4s[i])

static void RK(DdsProcessor* p);

EXPORT(int) DdsComputeDynamic(DDS_PROCESSOR ph,int method)
{
	ENTER(ph);

	ASSERT(VALUE(STEP()) > 0.0);

	if (method != 0) {
		if (method != DDS_I_EULER && method != DDS_I_RUNGE_KUTTA) THROW(DDS_ERROR_ARGUMENT, DDS_MSG_ARGUMENT);
	} else {
		method = METHOD();
		if (method == 0) method = DDS_I_RUNGE_KUTTA;
	}
	METHOD() = method;

	// Save current value

	if (STAGE() == 0) {
		double stp = VALUE(STEP());
		if(I_COUNT()>0) ISs = (double*)MemAlloc(sizeof(double) * I_COUNT());
		if (METHOD() == DDS_I_BW_EULER) VALUE(STEP()) = 0.0;
		for (int i = 0; i < I_COUNT(); ++i) {
			INDEX(IV(i)) = i;
		}
		for (int i = 0; i < I_COUNT(); ++i) IS(i) = VALUE(IV(i));
		STATUS() = DdsComputeStatic(ph);
		if (METHOD() == DDS_I_BW_EULER) VALUE(STEP()) = stp;
		return STATUS();
	}

	if (I_COUNT() <= 0) return 0;
	for (int i = 0; i < I_COUNT(); ++i) IS(i) = VALUE(IV(i));
	STAGE() = DDS_COMPUTED_EVERY_TIME;
	if (method == DDS_I_BW_EULER) {
		// Integration is included in the algebraic computation.
		STATUS() = DdsComputeStatic(ph);
	} else {
		// Integration: compute <I> at time = TIME()+STEP() 
		if (method == DDS_I_EULER) { for (int i = 0; i < I_COUNT(); ++i) VALUE(IV(i)) = FUNCTION(IV(i))(p, IV(i)); }
		else                         RK(p);
		// Compute static-surface at the time = TIME()+STEP()
		STATUS() = DdsComputeStatic(ph);
	}
	VALUE(TIME()) += VALUE(STEP());  // increment time
	STAGE() = DDS_COMPUTED_ANY_TIME;
	LEAVE(ph);
	return STATUS();
}
//
// Runge-Kutta integrator.
//
static void RK(DdsProcessor* p)
{
	int e = 0;
	double dt = VALUE(STEP());
	double time_save = VALUE(TIME());

	if (I_COUNT() <= 0) return;
	if (K1s == nullptr) {
		K1s = (double*)MemAlloc(sizeof(double) * I_COUNT());
		K2s = (double*)MemAlloc(sizeof(double) * I_COUNT());
		K3s = (double*)MemAlloc(sizeof(double) * I_COUNT());
		K4s = (double*)MemAlloc(sizeof(double) * I_COUNT());
	}

	// K1
	if (STAGE() == 0 ) e = DdsComputeStatic(p);
	if (e != 0) THROW(e, "ERROR: DdsComputeStatic() for solving RUNGE-KUTTA method(K1). ");
	for (int i = 0; i < I_COUNT(); ++i) {
		K1(i) = VALUE(RHSV(IV(i), 0));
	}

	// K2
	STAGE() = DDS_COMPUTED_EVERY_TIME;
	VALUE(TIME()) += dt / 2.0;
	for (int i = 0; i < I_COUNT(); ++i) {
		VALUE(IV(i)) = IS(i) + dt*K1(i)/2.0;
	}
	e = DdsComputeStatic(p);
	if (e != 0) THROW(e, "ERROR: DdsComputeStatic() for solving RUNGE-KUTTA method(K2). ");
	for (int i = 0; i < I_COUNT(); ++i) {
		K2(i) = VALUE(RHSV(IV(i), 0));
	}

	// K3
	STAGE() = DDS_COMPUTED_EVERY_TIME;
	for (int i = 0; i < I_COUNT(); ++i) {
		VALUE(IV(i)) = IS(i) + dt * K2(i) / 2.0;
	}
	e = DdsComputeStatic(p);
	if (e != 0) THROW(e, "ERROR: DdsComputeStatic() for solving RUNGE-KUTTA method(K3). ");
	for (int i = 0; i < I_COUNT(); ++i) {
		K3(i) = VALUE(RHSV(IV(i), 0));
	}

	// K4
	VALUE(TIME()) += dt / 2.0;
	STAGE() = DDS_COMPUTED_EVERY_TIME;
	for (int i = 0; i < I_COUNT(); ++i) {
		VALUE(IV(i)) = IS(i) + dt * K3(i);
	}
	e = DdsComputeStatic(p);
	if (e != 0) THROW(e, "ERROR: DdsComputeStatic() for solving RUNGE-KUTTA method(K4). ");
	for (int i = 0; i < I_COUNT(); ++i) {
		K4(i) = VALUE(RHSV(IV(i), 0));
	}

	for (int i = 0; i < I_COUNT(); ++i) {
		VALUE(IV(i)) = IS(i) + dt * (K1(i) + 2.0 * K2(i) + 2.0 * K3(i) + K4(i)) / 6.0;
	}
	VALUE(TIME()) = time_save;
}