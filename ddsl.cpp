/*
 *
 * DDSL: Digital Dynamic Simulation Library (C/C++ Library).
 *
 * Copyright(C) 2021 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */

/* References:
  Masao Iri, Junkichi Tsunekawa, Keiji Yajima:The Graphical Techniques Used for a Chemical Process Simulator "JUSE GIFS". IFIP Congress
  Dr. Kazuo Murota (auth.):Systems Analysis by Graphs and Matroids: Structural Solvability and Controllability
(https://jp1lib.org/book/2264352/362f6b?id=2264352&secret=362f6b)
 K.Yajima, J.Tsunekawa and S.Kobayashi(1981) : On equation - based dynamic simulation.Proc. Proceedings. 2nd World Congress of Chemical Engineering, 1981 Montreal, 1981
 K.Yajima, J.Tsunekawa, H.Shono, S.Kobayashi and D.J.Sebastian (1982) : On graph - theoretic techniques for large - scale process systems.
 The International symposium on process systems engineering,Kyoto International Conference Hall, Kyoto Japan, August 23-27, 1982
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
// Error handler ... see utils.h
//
EXPORT(ErrHandler)    DdsGetErrorHandler(DDS_PROCESSOR p)
{
	return p->ErrorHandler;
}
EXPORT(void)          DdsSetErrorHandler(DDS_PROCESSOR p, ErrHandler handler)
{
	p->ErrorHandler = handler;
}


EXPORT(int)  DdsCompileGraph(DDS_PROCESSOR p,int method)
{
	int e = 0;
	if (method == 0) method = DDS_I_RUNGE_KUTTA;// default method
	if (method != DDS_I_EULER && method != DDS_I_BW_EULER && method != DDS_I_RUNGE_KUTTA && method != DDS_STEADY_STATE) 
		return DDS_ERROR_ARGUMENT;
	p->method = method;
	if (method == DDS_I_BW_EULER|| method == DDS_STEADY_STATE) I_BACKTRACK() = 0;
	else                                                       I_BACKTRACK() = DDS_FLAG_INTEGRATED;

	STAGE() = 0;

	_D(DdsDbgPrintF(stdout, "Before processing:", p));
	e = DdsSieveVariable(p);
	_D(DdsDbgPrintF(stdout, "After DdsSieveVariable(p)", p));
	if (e) return e;
	e = DdsDivideLoop(p);
	_D(DdsDbgPrintF(stdout, "After DdsDivideLoop(p)", p));
	if (e) return e;
	e = DdsCheckRouteFT(p);
	_D(DdsDbgPrintF(stdout, "After DdsCheckRouteFT(p)", p));
	if (e) return e;
	e = DdsBuildSequence(p);
	_D(DdsDbgPrintF(stdout, "After DdsBuildSequence(p)", p));
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
		DdsProcessor* pr = (DdsProcessor*)MemAlloc(nullptr,sizeof(DdsProcessor));
		pr->Vars = (DdsVariable**)MemAlloc(pr,sizeof(DdsVariable*) * nv);
		pr->v_count = 0;
		pr->v_max   = nv;
		*p = (DDS_PROCESSOR)pr;
		// Add TIME and STEP
		DdsAddVariableV(pr, &pr->Time, "#TIME", DDS_FLAG_SET | DDS_FLAG_VOLATILE, 0.0, nullptr, 0);
		DdsAddVariableV(pr, &pr->Step, "#STEP", DDS_FLAG_SET | DDS_FLAG_VOLATILE, 0.0, nullptr, 0);
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

EXPORT(DDS_VARIABLE)  DdsTime(DDS_PROCESSOR ph)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	return TIME();
}
EXPORT(DDS_VARIABLE)  DdsStep(DDS_PROCESSOR ph)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	return STEP();
}

EXPORT(void*)         DdsGetProcessorUserPTR(DDS_PROCESSOR ph)
{
	return (void*)(*((void**)ph));
}
EXPORT(void)          DdsSetProcessorUserPTR(DDS_PROCESSOR ph, void* val)
{
	(*((void**)ph)) = val;
}
EXPORT(void*)         DdsGetVariableUserPTR(DDS_VARIABLE v)
{
	return (void*)(*((void**)v));
}
EXPORT(void)          DdsSetVariableUserPTR(DDS_VARIABLE v, void* val)
{
	(*((void**)v)) = val;
}


EXPORT(double)        DdsGetEPS(DDS_PROCESSOR ph)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	return EPS();
}
EXPORT(double)        DdsSetEPS(DDS_PROCESSOR ph, double eps)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	if (eps > 0) EPS() = eps;
	return EPS();
}

EXPORT(int)           DdsGetMaxIterations(DDS_PROCESSOR ph)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	return p->max_iter;
}
EXPORT(int)           DdsSetMaxIterations(DDS_PROCESSOR ph, int max)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	if (max > 0) p->max_iter = max;
	return p->max_iter;
}

EXPORT(void) DdsFreeWorkMemory(DDS_PROCESSOR ph)
{
	if (ph == nullptr) return;
	DdsProcessor* p = (DdsProcessor*)(ph);
	// free memories allocated at  DdsCheckRouteFT()
	if (TVs() != nullptr)          MemFree(p,(void**)&(TVs()));
	if (F_COUNTs() != nullptr)     MemFree(p,(void**)&(F_COUNTs()));
	if (F_MAX_COUNTs() != nullptr) MemFree(p,(void**)&(F_MAX_COUNTs()));
	for (int i = 0; i < T_COUNT(); ++i) {
		MemFree(p,(void**)&(Fs_CONNECTED(i)));
	}
	if (FT_MATRIX() != nullptr)    MemFree(p,(void**)&(FT_MATRIX()));
	// DdsBuildSequence()
	if (IVs() != nullptr)          MemFree(p,(void**)&(IVs()));
	// DdsComputeStatic()
	if (JACOBIAN_MATRIX() != nullptr)   MemFree(p,(void**)&(JACOBIAN_MATRIX()));
	if (DELTAs() != nullptr)       MemFree(p,(void**)&(DELTAs()));
	if (Xs() != nullptr)           MemFree(p,(void**)&(Xs()));
	if (DXs() != nullptr)          MemFree(p,(void**)&(DXs()));
	if (Ys() != nullptr)           MemFree(p,(void**)&(Ys()));
	if (Y_NEXTs() != nullptr)      MemFree(p,(void**)&(Y_NEXTs()));
	if (SCALE() != nullptr)        MemFree(p,(void**)&(SCALE()));
	if (PIVOT() != nullptr)        MemFree(p,(void**)&(PIVOT()));
	// RUNGE-KUTTA working arrays.
	if (p->R_K1  != nullptr)        MemFree(p,(void**)&(p->R_K1));
	if (p->R_K2  != nullptr)        MemFree(p,(void**)&(p->R_K2));
	if (p->R_K3  != nullptr)        MemFree(p,(void**)&(p->R_K3));
	if (p->R_K4  != nullptr)        MemFree(p,(void**)&(p->R_K4));
	if (p->R_IVS != nullptr)        MemFree(p,(void**)&(p->R_IVS));

	T_COUNT() = 0;
	B_COUNT() = 0;
	I_COUNT() = 0;
}

EXPORT(void) DdsDeleteProcessor(DDS_PROCESSOR* ph)
{
	if (ph == nullptr) return;
	DdsProcessor* p = (DdsProcessor*)(*ph);
	for(int i=0;i<VARIABLE_COUNT();++i) MemFree(p,(void**)&(VARIABLE(i)));
	MemFree(p,(void**)&(VARIABLEs()));
	
	DdsFreeWorkMemory(*ph);

	// Finally delete processor!
	MemFree(p,(void**)&(p));
	*ph = nullptr;
	_D(PrintAllocCount("DdsDeleteProcessor()"));
}

EXPORT(const char*)  DdsGetVariableName(DDS_VARIABLE v)
{
	return (const char*)NAME(((DdsVariable*)v));
}

EXPORT(double) DdsGetValue(DDS_VARIABLE v)
{
	return VALUE(((DdsVariable*)v));
}
EXPORT(double) DdsSetValue(DDS_VARIABLE v, double val)
{
	VALUE(((DdsVariable*)v)) = val;
	return VALUE(((DdsVariable*)v));
}


EXPORT(DDS_VARIABLE*) DdsGetVariables(int *nv,DDS_PROCESSOR ph)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	*nv = VARIABLE_COUNT();
	return (DDS_VARIABLE*)VARIABLEs();
}

EXPORT(DDS_VARIABLE*) DdsGetRhsvs(int* nr, DDS_VARIABLE v)
{
	DdsVariable* p = (DdsVariable*)v;
	*nr = RHSV_COUNT(p);
	return (DDS_VARIABLE*)(RHSVs(p));
}


static void AddVariable(DdsProcessor* p, DdsVariable* pv)
{
	if (p->v_count >= p->v_max) {
		p->Vars = (DdsVariable**)MemRealloc(p,p->Vars, (p->v_max += p->v_max / 9 + 5)*sizeof(DdsVariable*));
	}
	p->Vars[p->v_count++] = pv;
}

static DdsVariable* AllocVariable(DdsProcessor *p,const char* name, unsigned int f, double val, ComputeVal func, int nr)
{
	int l = strlen(name) + 3;
	DdsVariable* v = (DdsVariable*)MemAlloc(p,sizeof(DdsVariable) + l + (nr+ RHSV_EX()) * sizeof(DdsVariable*));
	v->Function = (ComputeVal)func;
	v->Value = val;
	v->UFlag = DdsSetUserFlagOn((DDS_VARIABLE)v, f);
	v->Nr = nr;
	v->Rhsvs = (VARIABLE**)((char*)v + sizeof(DdsVariable));
	v->Name = (char*)v->Rhsvs + sizeof(DdsVariable*) * (nr+ RHSV_EX());
	strcpy(v->Name, name);
	return v;
}

EXPORT(int)  DdsAddVariableA(DDS_PROCESSOR p, DDS_VARIABLE* pv, const char* name, unsigned int f, double val, ComputeVal func, int nr, DDS_VARIABLE* rhsvs)
{
	try {
		DDS_VARIABLE v = AllocVariable(p,name, f, val, func, nr);
		for (int i = 0; i < nr; ++i) {
			v->Rhsvs[i] = (DDS_VARIABLE)rhsvs[i];
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
		DdsVariable* v = AllocVariable(p,name,  f, val, func, nr);
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
		if (DDS_FLAG_OR(F, DDS_SFLAG_FREE))         fprintf(f, " F");   /* <F> */
		else                                        fprintf(f, " f");
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
		fprintf(f, "V:%lE ", pv->Value);
		fprintf(f, "S:%04d I:%04d]",pv->score,pv->index);
		if (pv->Function == nullptr) fprintf(f, " ?(");
		else                         fprintf(f, " f(");
		for (int j = 0; j < RHSV_COUNT(pv); ++j) {
			if (pv->Rhsvs[j] == nullptr) fprintf(f, "? ");
			else                         fprintf(f, "%s ", (pv->Rhsvs[j])->Name);
		}
		fprintf(f,")");
		if (pv->next != nullptr) fprintf(f, " N:%s", (pv->next)->Name);
		fprintf(f, "\n");
	}
}

EXPORT(DDS_VARIABLE)  DdsGetVariableNext(DDS_VARIABLE hv)
{
	return NEXT(hv);
}
EXPORT(int)           DdsGetVariableIndex(DDS_VARIABLE hv)
{
	return INDEX(hv);
}
EXPORT(int)           DdsGetVariableScore(DDS_VARIABLE hv)
{
	return SCORE(hv);
}

EXPORT(int) DdsSetRHSV(DDS_VARIABLE hv,int i,DDS_VARIABLE rv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	if (i < 0 || i >= RHSV_COUNT(pv)) return DDS_ERROR_INDEX;
	RHSV(pv, i) = (DdsVariable *)rv;
	return 0;
}

EXPORT(DDS_VARIABLE) DdsGetRHSV(DDS_VARIABLE hv, int i)
{
	DdsVariable* pv = (DdsVariable*)hv;
	if (i < 0 || i >= RHSV_COUNT(pv)) return (DDS_VARIABLE)nullptr;
	return RHSV(pv, i);
}


EXPORT(int) DdsCheckVariable(DDS_PROCESSOR ph,DDS_VARIABLE hv)
{
	DdsProcessor* p = ph;
	DdsVariable*  pv = (DdsVariable*)hv;
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
	// RHSVs of <I> must be (<DR>,Time,Step) Time & Step can be omitted.
	if (DDS_FLAG_OR(f, DDS_FLAG_INTEGRATED)) {
		if (RHSV_COUNT(pv) != 1) {
			if (RHSV_COUNT(pv) != 3)        return DDS_ERROR_FLAG;
			if (RHSV(pv, 1) != VARIABLE(0)) return DDS_ERROR_FLAG;
			if (RHSV(pv, 2) != VARIABLE(1)) return DDS_ERROR_FLAG;
		}
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

EXPORT(unsigned int) DdsSetUserFlagOn(DDS_VARIABLE hv, unsigned int f)
{
	DdsVariable* pv = (DdsVariable*)hv;
	USER_FLAG(pv) |= f; // Set bits for user.
	return USER_FLAG(pv);
}

EXPORT(unsigned int) DdsSetUserFlagOff(DDS_VARIABLE hv, unsigned int f)
{
	DdsVariable* pv = (DdsVariable*)hv;
	USER_FLAG(pv) &= (~f);
	return USER_FLAG(pv);
}

EXPORT(unsigned int) DdsGetSystemFlag(DDS_VARIABLE hv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	return SYS_FLAG(pv);
}

EXPORT(unsigned int) DdsGetUserFlag(DDS_VARIABLE hv)
{
	DdsVariable* pv = (DdsVariable*)hv;
	return USER_FLAG(pv);
}

EXPORT(unsigned int) DdsSetUserFlag(DDS_VARIABLE hv,unsigned int f)
{
	DdsVariable* pv = (DdsVariable*)hv;
	USER_FLAG(pv) = f;
	return USER_FLAG(pv);
}

EXPORT(DDS_VARIABLE)  DdsGetVariableSequence(DDS_PROCESSOR ph, unsigned int seq)
{
	DdsProcessor* p = (DdsProcessor*)ph;
	switch (seq) {
	case DDS_COMPUTED_ONCE:       return V_TOP_ONCET();
	case DDS_COMPUTED_EVERY_TIME: return V_TOP_EVERYT();
	case DDS_COMPUTED_ANY_TIME:   return V_TOP_ANYT();
	default:
		ASSERT(false);
	}
	return nullptr;
}
