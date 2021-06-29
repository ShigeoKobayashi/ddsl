#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ddsl.h"

typedef struct {
	int* J[8];
	DDS_VARIABLE* y;
	DDS_VARIABLE* x;
} U;

static double CompY(DDS_PROCESSOR p,DDS_VARIABLE y)
{
	U * u = (U*)DdsGetProcessorUserPTR(p);
	DDS_VARIABLE* x = u->x;
	double s = 0.0;
	int i, j;
	i = (int)DdsGetVariableUserPTR(y);
	for (j = 0; j < 8; ++j) {
		if (u->J[i][j] == 0) continue;
		s += (double)(u->J[i][j]) * DdsGetValue(x[j]);
	}
	return s;
}

static void Confirm(DDS_PROCESSOR p,DDS_VARIABLE* y, DDS_VARIABLE* x, int* J[8])
{
	int i;
	for (i = 0; i < 8; ++i) {
		printf("%d y-Jx=%lf\n", i, DdsGetValue(y[i]) - CompY(p, y[i]));
	}
}

/*
 *    y = Jx
 */
void Test2()
{
	/* X0 1 2 3 4 5 6 7
		1 0 0 1 0 0 1 1  x0 y0         A0 1 1 0 0 0 0 0 0  x4 y3
		1 0 0 0 0 1 1 1  x1 y1         A0 1 1 0 0 0 0 0 0  x3 y5
		0 0 0 0 0 1 0 1  x2 y2         A1 0 0 1 1 1 0 0 0  x7 y4
		0 0 0 1 1 0 0 0  x3 y3         A1 0 0 1 1 1 0 0 0  x2 y6
	 A  0 1 1 0 0 0 0 1  x4 y4   ==>   A1 0 0 1 1 1 0 0 0  x1 y7
		0 0 0 1 1 0 0 0  x5 y5         A2 0 0 1 0 0 1 0 0  x5 y2 
		0 1 1 0 0 0 0 1  x6 y6         A3 0 1 1 0 0 0 1 1  x6 y0
		0 1 1 0 0 0 0 1  x7 y7         A3 0 0 1 0 0 1 1 1  x0 y1

		Block triangular decomposition
		 {y3,y5}    {x3,x4}
		 {y4,y6,y7} {x1,x2,x7}
		 {y2}       {x5}
		 {y0,y1}    {x0,x6}

	*/
	int i,j;
	U u;
	DDS_VARIABLE X[8], Y[8];
	int J[][8] = {
		{1, 0, 0, 1, 0, 0, 1, 1}, /* 0 */
		{1, 0, 0, 0, 0, 1, 1, 1}, /* 1 */
		{0, 0, 0, 0, 0, 1, 0, 1}, /* 2 */
		{0, 0, 0, 1, 1, 0, 0, 0}, /* 3 */
		{0, 1, 1, 0, 0, 0, 0, 1}, /* 4 */
		{0, 0, 0, 1, 1, 0, 0, 0}, /* 5 */
		{0, 1, 1, 0, 0, 0, 0, 1}, /* 6 */
		{0, 1, 1, 0, 0, 0, 0, 1}  /* 7 */
	};
	char name[4];
	DDS_PROCESSOR p;
	DDS_VARIABLE A[8];
	int nr;

	DdsCreateProcessor(&p, 5);
	DdsSetProcessorUserPTR(p, J);

	for (i = 0; i < 8; ++i) {
		name[0] = 'X';
		name[1] = '0' + i;
		name[2] = 0;
		DdsAddVariableV(p, &X[i], name, DDS_FLAG_REQUIRED, 0.0, NULL, 0);
	}
	for (i = 0; i < 8; ++i) {
		name[0] = 'Y';
		name[1] = '0' + i;
		nr = 0;
		for (j = 0; j < 8; ++j) {
			if (J[i][j] == 0) continue;
			A[nr++] = X[j];
		}
		DdsAddVariableA(p, &Y[i], name, DDS_FLAG_TARGETED, (double)i, CompY, nr,A);
		DdsSetVariableUserPTR(Y[i], (void*)i);
	}
	for(i=0;i<8;++i) u.J[i] = J[i];
	u.x = X;
	u.y = Y;
	DdsSetProcessorUserPTR(p, (void*)&u);
	DdsCompileGraph(p, DDS_I_EULER);
	DdsComputeStatic(p);
	DdsDeleteProcessor(&p);
	printf("\n\n Enter any key to close: ");
	getchar();
}