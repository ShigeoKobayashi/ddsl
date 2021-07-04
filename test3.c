#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "ddsl.h"

/* defined in test.c */
extern void PrintSequence(DDS_PROCESSOR p);

static DDS_PROCESSOR p;
static DDS_VARIABLE I, dIdT, time, step, F1, M1, T1, F2, M2, T2, F3, M3, T3, R;
static double CompR(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return DdsGetValue(M3);
}
static double CompM1(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return DdsGetValue(R) - DdsGetValue(F1);
}
static double CompM2(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return DdsGetValue(M1)*exp(-DdsGetValue(I))+ DdsGetValue(F2);
}
static double CompM3(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return DdsGetValue(M2) - DdsGetValue(F3);
}
static double CompT1(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return 1.0-DdsGetValue(M1);
}
static double CompT2(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return DdsGetValue(I) - DdsGetValue(M2);
}
static double CompT3(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return 3.0 - DdsGetValue(M3);
}
static double CompDR(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	return 1.0 - DdsGetValue(M2);
}

void Test3()
{
	int i;
	DDS_VARIABLE * pR;
	DdsCreateProcessor(&p, 10);
	
	time = DdsTime(p);
	step = DdsStep(p);

	DdsAddVariableV(p, &I, "I", DDS_FLAG_REQUIRED | DDS_FLAG_INTEGRATED, 0.0, NULL,1, &dIdT);
	
	// If divided ==> unable to solve!
	DdsAddVariableV(p, &R, "R", DDS_FLAG_REQUIRED , 0.0, CompR, 1, &M3);

	DdsAddVariableV(p, &F1, "F1", DDS_FLAG_REQUIRED, 0.0, NULL,0);
	DdsAddVariableV(p, &F2, "F2", DDS_FLAG_REQUIRED, 0.0, NULL, 0);
	DdsAddVariableV(p, &F3, "F3", DDS_FLAG_REQUIRED, 0.0, NULL, 0);

	DdsAddVariableV(p, &M1, "M1", 0, 0.0, CompM1, 2,F1,R);
	DdsAddVariableV(p, &M2, "M2", 0, 0.0, CompM2, 3,I,M1,F2);
	DdsAddVariableV(p, &M3, "M3", DDS_FLAG_DIVISIBLE, 0.0, CompM3, 2,M2,F3);

	DdsAddVariableV(p, &T1, "T1", DDS_FLAG_TARGETED, 0.0, CompT1, 1,M1);
	DdsAddVariableV(p, &T2, "T2", DDS_FLAG_TARGETED, 0.0, CompT2, 2,M2,I);
	DdsAddVariableV(p, &T3, "T3", DDS_FLAG_TARGETED, 0.0, CompT3, 1,M3);

	DdsAddVariableV(p, &dIdT, "dI/dT", DDS_FLAG_REQUIRED, 0.0, CompDR, 1, M2);


	pR = DdsGetRhsvs(&i,I);
	pR[0] = dIdT;
	pR = DdsGetRhsvs(&i,R);
	pR[0] = M3;
	DdsSetValue(step, 0.1);

	DdsCompileGraph(p, 0);
	PrintSequence(p);
	for (i = 0; i < 10; ++i) {
		DdsComputeDynamic(p, 0);
		printf("T:%lf dIdT:%lf I:%lf R:%lf F1:%lf F2:%lf F3:%lf\n",
			DdsGetValue(time),
			DdsGetValue(dIdT),
			DdsGetValue(I),
			DdsGetValue(R),
			DdsGetValue(F1),
			DdsGetValue(F2),
			DdsGetValue(F3)
		);
	}
	DdsCompileGraph(p, DDS_STEADY_STATE);
	PrintSequence(p);
	DdsComputeStatic(p);
	printf("Steady state:\n");
	printf("T:%lf dIdT:%lf I:%lf R:%lf F1:%lf F2:%lf F3:%lf\n",
		DdsGetValue(time),
		DdsGetValue(dIdT),
		DdsGetValue(I),
		DdsGetValue(R),
		DdsGetValue(F1),
		DdsGetValue(F2),
		DdsGetValue(F3)
	);

	DdsDeleteProcessor(&p);
}