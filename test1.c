#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ddsl.h"

/*
 * Examples in ddsl_*.html 
 */

 /* defined in test.c */
extern void PrintInfo(DDS_PROCESSOR p);

/* defined in test1.c */
extern void TestNL2();
extern void TestI1();
extern void Test1();

void Test1()
{
	TestNL2();
	TestI1();
}

static
double CompY(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	DDS_VARIABLE x1 = DdsGetRHSV(y, 0);
	DDS_VARIABLE x2 = DdsGetRHSV(y, 1);
	double xv1 = DdsGetValue(x1);
	double xv2 = DdsGetValue(x2);
	return exp(xv1) - xv2;
}

void TestNL2()
{
	DDS_PROCESSOR p;
	DDS_VARIABLE y, x1, x2;
	DdsCreateProcessor(&p, 10);
	DdsAddVariableV(p, &x1, "x1", DDS_FLAG_SET, 1.0, NULL, 0); /* Register x1 to p. */
	DdsAddVariableV(p, &x2, "x2", DDS_FLAG_SET, 2.0, NULL, 0);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_REQUIRED, 0.0, CompY, 2, x1, x2);
	DdsCompileGraph(p, 0); /* Check relations and determine computation order. */
	PrintInfo(p);
	DdsComputeStatic(p);   /* Compute variable's value according to defined(by DdsCompileGraph()) order. */
	printf("Value: y=%lf x1=%lf x2=%lf\n", DdsGetValue(y), DdsGetValue(x1), DdsGetValue(x2));

	DdsSetUserFlag(x1, DDS_FLAG_REQUIRED);
	DdsSetUserFlag(y,  DDS_FLAG_TARGETED);
	DdsSetValue(y,0.0);
	DdsCompileGraph(p, 0);
	PrintInfo(p);
	DdsComputeStatic(p);
	printf("Value: y=%lf x1=%lf x2=%lf\n", DdsGetValue(y), DdsGetValue(x1), DdsGetValue(x2));

	DdsDeleteProcessor(&p);

}

static
double CompDYDT(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	DDS_VARIABLE A = DdsGetRHSV(y, 0);
	DDS_VARIABLE B = DdsGetRHSV(y, 1);
	DDS_VARIABLE C = DdsGetRHSV(y, 2);
	DDS_VARIABLE Y = DdsGetRHSV(y, 3);
	double AV = DdsGetValue(A);
	double BV = DdsGetValue(B);
	double CV = DdsGetValue(C);
	double yv = DdsGetValue(Y);
	return AV * (CV - yv) + BV;
}

void TestI1()
{
	/* dy/dt = A(C-y) + B */
	/* dy/dt(inf) = (AC+B)/A */
	/* Steady state y = 5.0 */
	DDS_PROCESSOR p;
	DDS_VARIABLE y, dydt, A, B, C;
	DDS_VARIABLE time, step;
	DDS_VARIABLE* pVs, * pVr;
	int i, j, nv, nr;

	DdsCreateProcessor(&p, 10);
	DdsAddVariableV(p, &A, "A", DDS_FLAG_SET, 1.0, NULL, 0);
	DdsAddVariableV(p, &B, "B", DDS_FLAG_SET, 2.0, NULL, 0);
	DdsAddVariableV(p, &C, "C", DDS_FLAG_SET, 3.0, NULL, 0);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_REQUIRED | DDS_FLAG_INTEGRATED, 1.0, NULL, 1, &dydt);
	DdsAddVariableV(p, &dydt, "dy/dt", DDS_FLAG_REQUIRED, 0.0, CompDYDT, 4, &A, &B, &C, &y);
	time = DdsTime(p);
	step = DdsStep(p);

	pVs = DdsGetVariables(&nv, p);
	for (i = 0; i < nv; ++i) {
		pVr = DdsGetRhsvs(&nr, pVs[i]);
		for (j = 0; j < nr; ++j) {
			pVr[j] = *((DDS_VARIABLE*)pVr[j]);
		}
	}

	DdsSetValue(step, 0.1);
	DdsCompileGraph(p, 0);
	PrintInfo(p);
	for (i = 0; i < 11; ++i) {
		DdsComputeDynamic(p, 0);
		printf("Dynamic:Time=%lf dydt=%lf y=%lf\n", DdsGetValue(time), DdsGetValue(dydt), DdsGetValue(y));
	}
	DdsCompileGraph(p, DDS_STEADY_STATE);
	PrintInfo(p);
	DdsComputeStatic(p);
	printf("Steady :Time=%lf dydt=%lf y=%lf\n", DdsGetValue(time), DdsGetValue(dydt), DdsGetValue(y));
	DdsDeleteProcessor(&p);
}