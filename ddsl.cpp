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


EXPORT(int)  DdsCompileGraph(DDS_PROCESSOR p)
{
	int e = 0;
	DdsDbgPrintF(stdout, "Before processing:", p);
	e = DdsSieveVariable(p);
	DdsDbgPrintF(stdout, "After DdsSieveVariable(p)", p);
	if (e) return e;
	e = DdsDivideLoop(p);
	DdsDbgPrintF(stdout, "After DdsDivideLoop(p)", p);
	if (e) return e;
	e = DdsCheckRouteFT(p);
	DdsDbgPrintF(stdout, "After DdsCheckRouteFT(p)", p);
	if (e) return e;

	return e;
}



#ifdef _DEBUG
extern void PrintAllocCount(const char *msg);
#endif

EXPORT(int)  DdsCreateProcessor(DDS_PROCESSOR* p, int nv)
{
	try {
		if (nv <= 0) nv = 10;
		DdsProcessor* pr = (DdsProcessor*)MemAlloc(sizeof(DdsProcessor));
		pr->Vars = (DdsVariable**)MemAlloc(sizeof(DdsVariable*) * nv);
		pr->v_count = 0;
		pr->v_max   = nv;
		*p = (DDS_PROCESSOR)pr;
	}
	catch (Exception &ex) {
		return ex.Code;
	}
	catch (...) {
		TRACE_EX(("DdsCreateProcessor() aborted"));
		return DDS_ERROR_SYSTEM;
	}
	return 0;
}

EXPORT(void) DdsDeleteProcessor(DDS_PROCESSOR* ph)
{
	if (ph == nullptr) return;
	DdsProcessor* p = (DdsProcessor*)(*ph);
	for(int i=0;i<VARIABLE_COUNT();++i) MemFree((void**)&(VARIABLE(i)));
	MemFree((void**)&(VARIABLEs()));
	
	// free memories allocated at  DdsCheckRouteFT()
	if (TVs() != nullptr)          MemFree((void**)&(TVs()));
	if (F_COUNTs() != nullptr)     MemFree((void**)&(F_COUNTs()));
	if (F_MAX_COUNTs() != nullptr) MemFree((void**)&(F_MAX_COUNTs()));
	for (int i = 0; i < T_COUNT(); ++i) {
		MemFree((void**)&(Fs_CONNECTED(i)));
	}
	if (FT_MATRIX() != nullptr)    MemFree((void**)&(FT_MATRIX()));


	// Finally delete processor!
	MemFree((void**)&(p));
	*ph = nullptr;
#ifdef _DEBUG
	PrintAllocCount("DdsDeleteProcessor()");
#endif
}

EXPORT(DDS_VARIABLE*) DdsVariables(int *nv,DDS_PROCESSOR p)
{
	DdsProcessor* pr = (DdsProcessor*)p;
	*nv = pr->v_count;
	return (DDS_VARIABLE*)(pr->Vars);
}

EXPORT(DDS_VARIABLE*) DdsRhsvs(int* nr, DDS_VARIABLE v)
{
	DdsVariable* p = (DdsVariable*)v;
	*nr = RHSV_COUNT(p);
	return (DDS_VARIABLE*)(RHSVs(p));
}


static void AddVariable(DdsProcessor* p, DdsVariable* pv)
{
	if (p->v_count >= p->v_max) {
		p->Vars = (DdsVariable**)MemRealloc(p->Vars, (p->v_max += p->v_max / 9 + 5)*sizeof(DdsVariable*));
	}
	p->Vars[p->v_count++] = pv;
}

static DdsVariable* AllocVariable(const char* name, unsigned int f, double val, ComputeVal func, int nr)
{
	int l = strlen(name) + 3;
	DdsVariable* v = (DdsVariable*)MemAlloc(sizeof(DdsVariable) + l + nr * sizeof(DdsVariable*));
	v->Function = (ComputeVal)func;
	v->Value = val;
	v->UFlag = DdsUserFlagOn((DDS_VARIABLE)v, f);
	v->Nr = nr;
	v->Rhsvs = (VARIABLE**)((char*)v + sizeof(DdsVariable));
	v->Name = (char*)v->Rhsvs + sizeof(DdsVariable*) * nr;
	strcpy(v->Name, name);
	return v;
}

EXPORT(int)  DdsAddVariableA(DDS_PROCESSOR p, DDS_VARIABLE* pv, const char* name, unsigned int f, double val, ComputeVal func, int nr, DDS_VARIABLE** rhsvs)
{
	try {
		DdsVariable* v = AllocVariable(name, f, val, func, nr);
		for (int i = 0; i < nr; ++i) {
			v->Rhsvs[i] = (VARIABLE*)rhsvs[i];
		}
		*pv = v;
		AddVariable((DdsProcessor*)p, v);
	}
	catch (Exception& ex) {
		return ex.Code;
	}
	catch (...) {
		TRACE_EX(("DdsAddVariableA() aborted"));
		return DDS_ERROR_SYSTEM;
	}
	return 0;
}

EXPORT(int)  DdsAddVariableV(DDS_PROCESSOR p, DDS_VARIABLE* pv, const char* name, unsigned int f, double val, ComputeVal func, int nr, ...)
{
	try {
		DdsVariable* v = AllocVariable(name,  f, val, func, nr);
		va_list args;
		va_start(args, nr);
		for (int i = 0; i < nr; ++i) {
			v->Rhsvs[i] = va_arg(args, DdsVariable*);
		}
		va_end(args);
		*pv = v;
		AddVariable((DdsProcessor*)p, v);
	}
	catch (Exception& ex) {
		return ex.Code;
	}
	catch (...) {
		TRACE_EX(("DdsAddVariableV() aborted"));
		return DDS_ERROR_SYSTEM;
	}
	return 0;
}

EXPORT(void) DdsDbgPrintF(FILE* f, const char* title, DDS_PROCESSOR p)
{
	DdsProcessor* pr = (DdsProcessor*)p;
	fprintf(f,"\n%s\n", title);
	fprintf(f, " Total variables=%d/%d\n", pr->v_count, pr->v_max);
	for (int i = 0; i < pr->v_count; ++i) {
		DdsVariable* pv = pr->Vars[i];
		if (pv == nullptr) {
			fprintf(f, " %5d) NULL!\n", i);
			continue;
		}
		unsigned int F = pv->UFlag;
		fprintf(f, " %5d)%5s[",i,pv->Name);
		if (DDS_FLAG_OR(F, DDS_FLAG_REQUIRED))      fprintf(f, "R");  /* <R> */
		else                                        fprintf(f, "r");
		if (DDS_FLAG_OR(F, DDS_FLAG_SET))           fprintf(f, "S");  /* <S> */
		else                                        fprintf(f, "s");
		if (DDS_FLAG_OR(F, DDS_FLAG_TARGETED))      fprintf(f, "T");  /* <T> */
		else                                        fprintf(f, "t");
		if (DDS_FLAG_OR(F, DDS_FLAG_INTEGRATED))    fprintf(f, "I");  /* <I> */
		else                                        fprintf(f, "i");
		if (DDS_FLAG_OR(F, DDS_FLAG_VOLATILE))      fprintf(f, "V");  /* <V> */
		else                                        fprintf(f, "v");
		if (DDS_FLAG_OR(F, DDS_FLAG_DIVISIBLE))     fprintf(f, "D");  /* <D> */
		else                                        fprintf(f, "d");
		if (DDS_FLAG_OR(F, DDS_FLAG_NON_DIVISIBLE)) fprintf(f, "N");  /* <N> */
		else                                        fprintf(f, "n");
		fprintf(f, "-");
		F = pv->SFlag;
		if (DDS_FLAG_OR(F, DDS_FLAG_REQUIRED))      fprintf(f, "R");  /* <R> */
		else                                        fprintf(f, "r");
		if (DDS_FLAG_OR(F, DDS_FLAG_SET))           fprintf(f, "S");  /* <S> */
		else                                        fprintf(f, "s");
		if (DDS_FLAG_OR(F, DDS_FLAG_TARGETED))      fprintf(f, "T");  /* <T> */
		else                                        fprintf(f, "t");
		if (DDS_FLAG_OR(F, DDS_FLAG_INTEGRATED))    fprintf(f, "I");  /* <I> */
		else                                        fprintf(f, "i");
		if (DDS_FLAG_OR(F, DDS_FLAG_VOLATILE))      fprintf(f, "V");  /* <V> */
		else                                        fprintf(f, "v");
		if (DDS_FLAG_OR(F, DDS_FLAG_DIVISIBLE))     fprintf(f, "D");  /* <D> */
		else                                        fprintf(f, "d");
		if (DDS_FLAG_OR(F, DDS_FLAG_NON_DIVISIBLE)) fprintf(f, "N");  /* <N> */
		else                                        fprintf(f, "n");
		if (DDS_FLAG_OR(F, DDS_SFLAG_ALIVE))        fprintf(f, " AL");  /* <AL> */
		else                                        fprintf(f, " al");
		if (DDS_FLAG_OR(F, DDS_SFLAG_FREE))         fprintf(f, " FR");  /* <AL> */
		else                                        fprintf(f, " fr");
		if (DDS_FLAG_OR(F, DDS_SFLAG_DIVIDED))      fprintf(f, " DD");  /* <DD> */
		else                                        fprintf(f, " dd");
		if (DDS_FLAG_OR(F, DDS_SFLAG_DERIVATIVE))   fprintf(f, " DR");  /* <DR> */
		else                                        fprintf(f, " dr");
		if (DDS_FLAG_OR(F, DDS_SFLAG_ERROR))        fprintf(f, " ER");  /* <ER> */
		else                                        fprintf(f, " er");
		if (DDS_FLAG_OR(F, DDS_COMPUTED_ONCE))      fprintf(f, " 1T");  /* <1T> */
		else                                        fprintf(f, " 1t");
		if (DDS_FLAG_OR(F, DDS_COMPUTED_ANY_TIME))  fprintf(f, " AT");  /* <AT> */
		else                                        fprintf(f, " at");
		if (DDS_FLAG_OR(F, DDS_COMPUTED_EVERY_TIME))fprintf(f, " ET |");  /* <ET> */
		else                                        fprintf(f, " et |");
		if (DDS_FLAG_OR(F, DDS_SFLAG_CHECKED))      fprintf(f, " CH ");  /* Checked */
		else                                        fprintf(f, " ch ");

		fprintf(f, "]{S%04d:I%04d}",pv->score,pv->index);
		if (pv->Function == nullptr) fprintf(f, "?(");
		else                         fprintf(f, "f(");
		for (int j = 0; j < RHSV_COUNT(pv); ++j) {
			if (pv->Rhsvs[j] == nullptr) fprintf(f, "? ");
			else                         fprintf(f, "%s ", (pv->Rhsvs[j])->Name);
		}
		fprintf(f,")");
		if (pv->next != nullptr) fprintf(f, "->%s", (pv->next)->Name);
		fprintf(f,"=%E", pv->Value);
		fprintf(f, "\n");
	}
}

EXPORT(int) DdsSetRHSV(DDS_VARIABLE hv,int i,DDS_VARIABLE rv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	if (i < 0 || i >= RHSV_COUNT(pv)) return DDS_ERROR_INDEX;
	RHSV(pv, i) = (DdsVariable *)rv;
	return 0;
}

EXPORT(DDS_VARIABLE) DdsGetRHSV(DDS_VARIABLE hv, int i, DDS_VARIABLE rv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	if (i < 0 || i >= RHSV_COUNT(pv)) return (DDS_VARIABLE)nullptr;
	return RHSV(pv, i);
}


EXPORT(int) DdsCheckVariable(DDS_VARIABLE hv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	unsigned int f = USER_FLAG(pv) & DDS_FLAG_MASK;

	SET_SFLAG_ON(pv, DDS_SFLAG_ERROR);
	if (DDS_FLAG_OR(f, DDS_FLAG_SET)) {
		if (DDS_FLAG_OR(f, DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED)) return DDS_ERROR_FLAG;
	}
	if (DDS_FLAG_AND(f, DDS_FLAG_INTEGRATED | DDS_FLAG_TARGETED)) return DDS_ERROR_FLAG;
	if (DDS_FLAG_AND(f, DDS_FLAG_DIVISIBLE | DDS_FLAG_NON_DIVISIBLE)) return DDS_ERROR_FLAG;
	if (DDS_FLAG_OR(f, DDS_FLAG_TARGETED)) {
		if(RHSV_COUNT(pv)<=0 || pv->Function==nullptr) return DDS_ERROR_FLAG;
	}
	if (DDS_FLAG_OR(f, DDS_FLAG_INTEGRATED)) {
		if (RHSV_COUNT(pv) != 1) return DDS_ERROR_FLAG;
	}
	if (DDS_FLAG_OR(f, DDS_FLAG_VOLATILE)) {
		if (!DDS_FLAG_OR(f, DDS_FLAG_SET | DDS_FLAG_TARGETED)) return DDS_ERROR_FLAG;
	}
	for (int i = 0; i < RHSV_COUNT(pv); ++i) {
		if (RHSV(pv, i) == nullptr) return DDS_ERROR_NULL;
	}
	SET_SFLAG_OFF(pv, DDS_SFLAG_ERROR);
	return 0;
}

EXPORT(unsigned int) DdsUserFlagOn(DDS_VARIABLE hv, unsigned int f)
{
	DdsVariable* pv = (DdsVariable*)hv;
	USER_FLAG(pv) |= f; // Set bits for user.
	return USER_FLAG(pv);
}

EXPORT(unsigned int) DdsUserFlagOff(DDS_VARIABLE hv, unsigned int f)
{
	DdsVariable* pv = (DdsVariable*)hv;
	USER_FLAG(pv) &= (~f);
	return USER_FLAG(pv);
}

EXPORT(unsigned int) DdsSystemFlag(DDS_VARIABLE hv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	return SYS_FLAG(pv);
}
