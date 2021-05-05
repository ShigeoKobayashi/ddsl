#include <stdio.h>
#include <stdlib.h>
#include "ddsl.h"

double CompVal(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	return 0.0;
}

#define NV 10
void Test1()
{
	int i,j,nr;
	DDS_PROCESSOR p;
	DDS_VARIABLE  V[NV];
	DDS_VARIABLE* pV;
	DDS_VARIABLE X[8], Y[8];
	char name[4];
	DdsCreateProcessor(&p, 5);
	/*
	DdsAddVariableV(p, &V[0], "0", 0, 0.0, CompVal, 1, &V[1]);
	DdsAddVariableV(p, &V[1], "1", 0, 0.0, CompVal, 1, &V[3]);
	DdsAddVariableV(p, &V[2], "2", 0, 0.0, CompVal, 1, &V[0]);
	DdsAddVariableV(p, &V[3], "3", 0, 0.0, CompVal, 2, &V[2], &V[5]);
	DdsAddVariableV(p, &V[4], "4", 0, 0.0, CompVal, 1, &V[3]);
	DdsAddVariableV(p, &V[5], "5", 0, 0.0, CompVal, 1, &V[6]);
	DdsAddVariableV(p, &V[6], "6", 0, 0.0, CompVal, 2, &V[4], &V[7]);
	DdsAddVariableV(p, &V[7], "7", 0, 0.0, CompVal, 1, &V[9]);
	DdsAddVariableV(p, &V[8], "8", 0, 0.0, CompVal, 1, &V[6]);
	DdsAddVariableV(p, &V[9], "9", 0, 0.0, CompVal, 1, &V[8]);
	for (i = 0; i < NV; ++i) {
		pV = DdsRhsvs(&nr, V[i]);
		for (j = 0; j < nr; ++j) {
			pV[j] = *((DDS_VARIABLE*)pV[j]);
		}
	}
	DdsUserFlagOn(V[0], DDS_FLAG_REQUIRED);
	*/

	/* X0 1 2 3 4 5 6 7
	    1 0 0 1 0 0 1 1  x0 y0         A0 1 1 0 0 0 0 0 0  x4 y3
		1 0 0 0 0 1 1 1  x1 y1         A0 1 1 0 0 0 0 0 0  x3 y5
		0 0 0 0 0 1 0 1  x2 y2         A1 0 0 1 1 1 0 0 0  x7 y4
		0 0 0 1 1 0 0 0  x3 y3         A1 0 0 1 1 1 0 0 0  x2 y6
	 A  0 1 1 0 0 0 0 1  x4 y4   ==>   A1 0 0 1 1 1 0 0 0  x1 y7
		0 0 0 1 1 0 0 0  x5 y5         A2 0 0 1 0 0 1 0 0  x5 y2
		0 1 1 0 0 0 0 1  x6 y6         A3 0 1 1 0 0 0 1 1  x6 y0
		0 1 1 0 0 0 0 1  x7 y7         A3 0 0 1 0 0 1 1 1  x0 y1
	*/
	name[0] = 'X';
	name[1] = '0';
	name[2] =  0;
	for (i = 0; i < 8; ++i) {
		DdsAddVariableV(p, &X[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 0);
		name[1] += 1;
	}
	name[0] = 'Y';
	name[1] = '0';
	DdsAddVariableV(p, &Y[0], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, X[0], X[3], X[6], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[1], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, X[0], X[5], X[6], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[2], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[5], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[3], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[4], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[5], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[6], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[7], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;

	DdsDbgPrintF(stdout, "Before Processing", p);
	DdsCompileGraph(p);
	DdsDbgPrintF(stdout, "After DdsCompileGraph() ", p);

	DdsDeleteProcessor(&p);
	getchar();
}

int main()
{
	Test1();
}
