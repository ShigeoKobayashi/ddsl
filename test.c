#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ddsl.h"

double CompVal(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	return 0.0;
}

#define NV 10
void Test1()
{
	int i, j, nr;
	DDS_VARIABLE  V[NV];
	DDS_VARIABLE* pV;

	printf("=== Test1()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
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
	DdsSetUserFlagOn(V[0], DDS_FLAG_REQUIRED);
	DdsCompileGraph(p,DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}

#define NV1 13
void Test1_1()
{
	int i, j, nr;
	DDS_VARIABLE  V[NV1];
	DDS_VARIABLE* pV;

	printf("=== Test1_1()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
	DdsAddVariableV(p, &V[0], "0", 0, 0.0, CompVal, 1, &V[1]);
	DdsAddVariableV(p, &V[1], "1", 0, 0.0, CompVal, 1, &V[3]);
	DdsAddVariableV(p, &V[2], "2", 0, 0.0, CompVal, 1, &V[0]);
	DdsAddVariableV(p, &V[3], "3", 0, 0.0, CompVal, 2, &V[2], &V[5]);
	DdsAddVariableV(p, &V[4], "4", 0, 0.0, CompVal, 1, &V[3]);
	DdsAddVariableV(p, &V[5], "5", 0, 0.0, CompVal, 1, &V[6]);
	DdsAddVariableV(p, &V[6], "6", 0, 0.0, CompVal, 2, &V[4], &V[7]);
	DdsAddVariableV(p, &V[7], "7", 0, 0.0, CompVal, 1, &V[9]);
	DdsAddVariableV(p, &V[8], "8", 0, 0.0, CompVal, 1, &V[6]);
	DdsAddVariableV(p, &V[9], "9", 0, 0.0, CompVal, 2, &V[8],&V[10]);
	DdsAddVariableV(p, &V[10], "10", 0, 0.0, CompVal, 1, &V[12]);
	DdsAddVariableV(p, &V[11], "11", 0, 0.0, CompVal, 1, &V[9]);
	DdsAddVariableV(p, &V[12], "12", 0, 0.0, CompVal, 1, &V[11]);

	for (i = 0; i < NV1; ++i) {
		pV = DdsRhsvs(&nr, V[i]);
		for (j = 0; j < nr; ++j) {
			pV[j] = *((DDS_VARIABLE*)pV[j]);
		}
	}
	DdsSetUserFlagOn(V[0], DDS_FLAG_REQUIRED);
	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}

void Test2()
{
	/* X0 1 2 3 4 5 6 7
		1 0 0 1 0 0 1 1  x0 y0         A0 1 1 0 0 0 0 0 0  x4 y3
		1 0 0 0 0 1 1 1  x1 y1         A0 1 1 0 0 0 0 0 0  x3 y5
		0 0 0 0 0 1 0 1  x2 y2         A1 0 0 1 1 1 0 0 0  x7 y4
		0 0 0 1 1 0 0 0  x3 y3         A1 0 0 1 1 1 0 0 0  x2 y6
	 A  0 1 1 0 0 0 0 1  x4 y4   ==>   A1 0 0 1 1 1 0 0 0  x1 y7
		0 0 0 1 1 0 0 0  x5 y5         A2 0 0 1 0 0 1 0 0  x5 y2 <V>
		0 1 1 0 0 0 0 1  x6 y6         A3 0 1 1 0 0 0 1 1  x6 y0
		0 1 1 0 0 0 0 1  x7 y7         A3 0 0 1 0 0 1 1 1  x0 y1
	*/
	int i;
	DDS_VARIABLE X[8], Y[8];
	char name[4];
	printf("=== Test2()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
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
	DdsAddVariableV(p, &Y[2], name, DDS_FLAG_TARGETED|DDS_FLAG_VOLATILE, 0.0, CompVal, 2, X[5], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[3], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[4], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[5], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[6], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[7], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}

void Test2_1()
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
	*/
	int i;
	DDS_VARIABLE X[8], Y[8],Z[8];
	char name[4];
	printf("=== Test2_1()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
	name[0] = 'X';
	name[1] = '0';
	name[2] = 0;
	for (i = 0; i < 8; ++i) {
		name[0] = 'X';
		DdsAddVariableV(p, &X[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 0);
		name[0] = 'Z';
		DdsAddVariableV(p, &Z[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 1,X[i]);
		name[0] = 'X';
		name[1] += 1;
	}
	name[0] = 'Y';
	name[1] = '0';
	DdsAddVariableV(p, &Y[0], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, Z[0], Z[3], Z[6], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[1], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, Z[0], Z[5], Z[6], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[2], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[5], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[3], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[3], Z[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[4], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[5], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[3], Z[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[6], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[7], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;
	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}

void Test2_2()
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
	*/
	int i;
	DDS_VARIABLE X[8], Y[8], Z[8],W[8];
	DDS_VARIABLE I, DE,* pV;
	int nr;

	char name[4];
	printf("=== Test2_2()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);

	DdsAddVariableV(p, &I, "I", DDS_FLAG_SET|DDS_FLAG_VOLATILE, 0.0, CompVal, 1,&DE);

	name[0] = 'X';
	name[1] = '0';
	name[2] = 0;
	for (i = 0; i < 8; ++i) {
		name[0] = 'X';
		DdsAddVariableV(p, &X[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 0);
		name[0] = 'W';
		if(i!=3)
			DdsAddVariableV(p, &W[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 1,X[i]);
		else
			DdsAddVariableV(p, &W[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 2, X[i],I);
		name[0] = 'Z';
		DdsAddVariableV(p, &Z[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 1, W[i]);
		name[0] = 'X';
		name[1] += 1;
	}
	name[0] = 'Y';
	name[1] = '0';
	DdsAddVariableV(p, &Y[0], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, Z[0], Z[3], Z[6], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[1], name, DDS_FLAG_TARGETED, 0.0, CompVal, 4, Z[0], Z[5], Z[6], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[2], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[5], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[3], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[3], Z[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[4], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[5], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, Z[3], Z[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[6], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[7], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, Z[1], Z[2], Z[7]); name[1] += 1;

	DdsAddVariableV(p, &DE, "DE", 0, 0.0, CompVal, 1, W[3]); 
	pV = DdsRhsvs(&nr, I);
	pV[0] = *((DDS_VARIABLE*)pV[0]);


	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}


void Test3()
{
	DDS_VARIABLE v1, v2, v3, v4, v5, v6, v7, v8;
	DDS_VARIABLE v11, v12, v13, v14, v15, v16;
	DDS_VARIABLE v21, v22, v23, v24, v25, v26;

	printf("=== Test3()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
	DdsAddVariableV(p, &v8, "8", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &v6, "6", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &v15, "15", DDS_FLAG_REQUIRED, 0.0, NULL, 0);
	DdsAddVariableV(p, &v25, "25", DDS_FLAG_SET, 0.0, NULL, 0);

	DdsAddVariableV(p, &v7, "7", 0, 0.0, CompVal, 1,v8);
	DdsAddVariableV(p, &v5, "5", 0, 0.0, CompVal, 1, v6);
	DdsAddVariableV(p, &v14, "14", 0, 0.0, CompVal, 1, v15);
	DdsAddVariableV(p, &v24, "24", 0, 0.0, CompVal, 1, v25);

	DdsAddVariableV(p, &v4, "4", 0, 0.0, CompVal, 1, v5);
	DdsAddVariableV(p, &v3, "3", 0, 0.0, CompVal, 1, v4);
	DdsAddVariableV(p, &v16, "16", 0, 0.0, CompVal, 1, v4);
	DdsAddVariableV(p, &v13, "13", 0, 0.0, CompVal, 2, v14, v16);
	DdsAddVariableV(p, &v26, "26", 0, 0.0, CompVal, 1, v13);
	DdsAddVariableV(p, &v23, "23", 0, 0.0, CompVal, 2, v26, v24);
	DdsAddVariableV(p, &v12, "12", 0, 0.0, CompVal, 2, v16, v13);
	DdsAddVariableV(p, &v2,  "2",  0, 0.0, CompVal, 2, v7, v3);
	DdsAddVariableV(p, &v22, "22", 0, 0.0, CompVal, 1, v23);

	DdsAddVariableV(p, &v1,   "1", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v2);
	DdsAddVariableV(p, &v11, "11", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v12);
	DdsAddVariableV(p, &v21, "21", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v22);
	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}

void Test3_1()
{
	DDS_VARIABLE v1, v2, v3, v4, v5, v6, v7, v8;
	DDS_VARIABLE v11, v12, v13, v14, v15, v16;
	DDS_VARIABLE v21, v22, v23, v24, v25, v26;
	DDS_VARIABLE a, b, c;
	DDS_VARIABLE A, B, C;

	printf("=== Test3_1()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
	DdsAddVariableV(p, &v8, "8", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &v6, "6", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &v15, "15", DDS_FLAG_REQUIRED, 0.0, NULL, 0);
	DdsAddVariableV(p, &v25, "25", DDS_FLAG_SET, 0.0, NULL, 0);

	DdsAddVariableV(p, &v7, "7", 0, 0.0, CompVal, 1, v8);
	DdsAddVariableV(p, &v5, "5", 0, 0.0, CompVal, 1, v6);
	DdsAddVariableV(p, &v14, "14", 0, 0.0, CompVal, 1, v15);
	DdsAddVariableV(p, &v24, "24", 0, 0.0, CompVal, 1, v25);

	DdsAddVariableV(p, &v4, "4", 0, 0.0, CompVal, 1, v5);
	DdsAddVariableV(p, &v3, "3", 0, 0.0, CompVal, 1, v4);
	DdsAddVariableV(p, &v16, "16", 0, 0.0, CompVal, 1, v4);
	DdsAddVariableV(p, &B, "B", 0, 0.0, CompVal, 1, v16);
	DdsAddVariableV(p, &b, "b", 0, 0.0, CompVal, 1, B);
	DdsAddVariableV(p, &v13, "13", 0, 0.0, CompVal, 2, v14, b);
	DdsAddVariableV(p, &v26, "26", 0, 0.0, CompVal, 1, v13);

	DdsAddVariableV(p, &v23, "23", 0, 0.0, CompVal, 2, v26, v24);

	DdsAddVariableV(p, &C, "C", 0, 0.0, CompVal, 1, v16);
	DdsAddVariableV(p, &c, "c", 0, 0.0, CompVal, 1, C);
	DdsAddVariableV(p, &A, "A", 0, 0.0, CompVal, 1, v13);
	DdsAddVariableV(p, &a, "a", 0, 0.0, CompVal, 1, A);
	DdsAddVariableV(p, &v12, "12", 0, 0.0, CompVal, 2, c, a);
	DdsAddVariableV(p, &v2, "2", 0, 0.0, CompVal, 2, v7, v3);
	DdsAddVariableV(p, &v22, "22", 0, 0.0, CompVal, 1, v23);

	DdsAddVariableV(p, &v1, "1", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v2);
	DdsAddVariableV(p, &v11, "11", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v12);
	DdsAddVariableV(p, &v21, "21", DDS_FLAG_TARGETED, 0.0, CompVal, 1, v22);
	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}


void Test0()
{
	DDS_PROCESSOR p;
	int nr;
	DDS_VARIABLE* pV;
	DDS_VARIABLE f1, f2, f3, x1, x2, x3, t1, t2, t3;
	DDS_VARIABLE c0, c1,c12, c23, c3;
	DDS_VARIABLE I, DE,I2,DE2;

	printf("=== Test0()---\n");
	DdsCreateProcessor(&p, 5);

	DdsAddVariableV(p, &c0, "c0", DDS_FLAG_SET|DDS_FLAG_VOLATILE, 0.0, NULL, 0);
	DdsAddVariableV(p, &f1, "f1", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &f2, "f2", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &f3, "f3", 0, 0.0, NULL, 0);
	DdsAddVariableV(p, &x1, "x1", 0, 0.0, CompVal, 2, f1,c0);
	DdsAddVariableV(p, &t1, "t1", DDS_FLAG_TARGETED|DDS_FLAG_VOLATILE, 0.0, CompVal, 1, x1);
	DdsAddVariableV(p, &c1, "c1", DDS_FLAG_REQUIRED, 0.0, CompVal, 1, t1);

	DdsAddVariableV(p, &I, "I", DDS_FLAG_INTEGRATED, 0.0, NULL, 1, &DE);
	DdsAddVariableV(p, &c12, "c12", 0, 0.0, CompVal, 2, I,x1);
	DdsAddVariableV(p, &x2, "x2", 0, 0.0, CompVal, 2, f2, c12);
	DdsAddVariableV(p, &t2, "t2", DDS_FLAG_TARGETED, 0.0, CompVal, 1, x2);
	DdsAddVariableV(p, &DE, "DE", 0, 0.0, CompVal, 1, c12);

	DdsAddVariableV(p, &I2, "I2", DDS_FLAG_INTEGRATED, 0.0, NULL, 1, &DE2);
	DdsAddVariableV(p, &x3, "x3", 0, 0.0, CompVal, 2, f3,I2);
	DdsAddVariableV(p, &c3, "c3", DDS_FLAG_REQUIRED, 0.0, CompVal, 1, x3);
	DdsAddVariableV(p, &c23, "c23", DDS_FLAG_REQUIRED, 0.0, CompVal, 2, x2, x3);
	DdsAddVariableV(p, &t3, "t3", DDS_FLAG_TARGETED, 0.0, CompVal, 1, x3);
	DdsAddVariableV(p, &DE2, "DE2", 0, 0.0, CompVal, 1, x3);


	pV = DdsRhsvs(&nr, I);
	pV[0] = *((DDS_VARIABLE*)pV[0]);
	pV = DdsRhsvs(&nr, I2);
	pV[0] = *((DDS_VARIABLE*)pV[0]);

	DdsCompileGraph(p, DDS_I_EULER);
	DdsDeleteProcessor(&p);
	getchar();
}



double a1[] = { 33.0, 16.0, 72.0 };
double a2[] = {-24.0,-10.0,-57.0 };
double a3[] = { -8.0, -4.0,-17.0 };
double* A[] = { a1,a2,a3 };
double B[3] = { -359.0, 281.0, 85.0 };
double ans[] = { -1.0, 2.0, 3.0 };

double CompVal1(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	int nr;
	DdsVariable* pv = (DdsVariable*)v;
	DdsVariable** pVs = DdsRhsvs(&nr,pv);
	return 1.0-2.0*DdsGetValue(pVs[0])- DdsGetValue(pVs[1]);
}

void TestDD()
{
	int i, j;

	DDS_PROCESSOR p;
	DDS_VARIABLE y0, y1, dy1;
	DDS_VARIABLE* pVs, * pVr;
	int nv, nr;

	DdsCreateProcessor(&p, 1);
	DdsAddVariableV(p, &y0, "y0", DDS_FLAG_INTEGRATED, 0.0, NULL, 1, &y1);
	DdsAddVariableV(p, &y1, "y1", DDS_FLAG_INTEGRATED, 0.0, NULL, 1, &dy1);
	DdsAddVariableV(p, &dy1, "dy1", DDS_FLAG_REQUIRED, 0.0, CompVal1, 2, &y1, &y0);


	pVs = DdsVariables(&nv, p);
	for (i = 0; i < nv; ++i) {
		pVr = DdsRhsvs(&nr, pVs[i]);
		for (j = 0; j < nr; ++j) {
			pVr[j] = *((DDS_VARIABLE*)pVr[j]);
		}
	}

	DdsCompileGraph(p, DDS_I_BW_EULER);

	DdsSetValue(DdsStep(p), 0.5);
	for (i = 0; i < 21; ++i) {
		DdsComputeDynamic(p, 0);
		printf(" %d) Time=%lf,Y=%lf\n", i, DdsGetValue(DdsTime(p)), DdsGetValue(y0));
	}
	DdsDbgPrintF(stdout, "After DdsComputeDynamic(p)", p);

	DdsDeleteProcessor(&p);
	getchar();
	return;
/*
	Test1();
	Test1_1();
	Test2();
	Test2_1();
	Test2_2();
	Test3();
	Test3_1();
*/
}


double CompY1(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	/*
	DDS_VARIABLE x = DdsGetRHSV(v, 0);
	double xv = DdsGetValue(x);
	return xv*xv;
	*/
	DDS_VARIABLE t = DdsGetRHSV(v, 0);
	double tv = DdsGetValue(t);
	return exp(-25.0 * tv);
}

double CompX(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE y = DdsGetRHSV(v, 0);
	double yv = DdsGetValue(y);
	DDS_VARIABLE x = DdsGetRHSV(v, 1);
	double xv = DdsGetValue(x);
	return yv-xv+1.0;
}

double CompDY(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE y = DdsGetRHSV(v, 0);
	double yv = DdsGetValue(y);
	return -25.0*yv;
}


void TestI()
{
	int i, j;

	DDS_PROCESSOR p;
	DDS_VARIABLE time,step,y,yt,dy;
	DDS_VARIABLE* pVs, * pVr;
	int nv, nr;

	DdsCreateProcessor(&p, 1);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_REQUIRED|DDS_FLAG_INTEGRATED, 1.0, NULL, 1, &dy);
	DdsAddVariableV(p, &dy, "dy", 0, 1.0, CompDY, 1, &y);
	DdsAddVariableV(p, &yt, "Y", DDS_FLAG_REQUIRED, 0.0, CompY1, 1, &time);
	time = DdsTime(p);
	step = DdsStep(p);
	pVs = DdsVariables(&nv, p);
	for (i = 0; i < nv; ++i) {
		pVr = DdsRhsvs(&nr, pVs[i]);
		for (j = 0; j < nr; ++j) {
			pVr[j] = *((DDS_VARIABLE*)pVr[j]);
		}
	}
	DdsSetValue(step, 0.01);
//	DdsCompileGraph(p, DDS_I_BW_EULER);
//	DdsCompileGraph(p, DDS_I_RUNGE_KUTTA);
	DdsCompileGraph(p, DDS_I_EULER);
	for (i = 0; i < 10; ++i) {
		DdsComputeDynamic(p,0);
		DdsComputeStatic(p);
		printf("T=%lf y=%lf Y=%lf\n", DdsGetValue(time), DdsGetValue(y), DdsGetValue(yt));
	}
	DdsDbgPrintF(stdout, "After DdsComputeStatic(p)", p);

	DdsDeleteProcessor(&p);
	getchar();
	return;
	/*
		Test1();
		Test1_1();
		Test2();
		Test2_1();
		Test2_2();
		Test3();
		Test3_1();
	*/
}

double CompF1(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE x2 = DdsGetRHSV(v, 0);
	DDS_VARIABLE y2 = DdsGetRHSV(v, 1);
	double s = DdsGetValue(x2) + DdsGetValue(y2) - 3.0;
	return s;
}
double CompF2(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE SinX = DdsGetRHSV(v, 0);
	DDS_VARIABLE ExpY = DdsGetRHSV(v, 1);
	double s = DdsGetValue(SinX) + DdsGetValue(ExpY) - 1.0;
	return s;
}
double CompX2(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE x = DdsGetRHSV(v, 0);
	double s = DdsGetValue(x) - 3.0;
	return s*s;
}
double CompY2(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE y = DdsGetRHSV(v, 0);
	double s = DdsGetValue(y);
	return s * s;
}
double CompSinX(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE x = DdsGetRHSV(v, 0);
	double s = DdsGetValue(x);
	return sin(s);
}
double CompExpY(DDS_PROCESSOR p, DDS_VARIABLE v)
{
	DDS_VARIABLE y = DdsGetRHSV(v, 0);
	double s = DdsGetValue(y)-1.0;
	return exp(s);
}
void TestNL()
{
	int i, j;
	DDS_PROCESSOR p;
	DDS_VARIABLE F1,F2,X2,Y2,SINX,EXPY,x,y;
	DDS_VARIABLE* pVs, * pVr;
	int nv, nr;

	DdsCreateProcessor(&p, 1);
	DdsAddVariableV(p, &F1, "F1", DDS_FLAG_TARGETED, 0.0, CompF1,    2, &X2,&Y2);
	DdsAddVariableV(p, &F2, "F2", DDS_FLAG_TARGETED, 0.0, CompF2,    2, &SINX, &EXPY);
	DdsAddVariableV(p, &X2, "X2", 0                , 0.0, CompX2,    1, &x);
	DdsAddVariableV(p, &Y2, "Y2", 0                , 0.0, CompY2,    1, &y);
	DdsAddVariableV(p, &SINX, "SINX", 0            , 0.0, CompSinX , 1, &x);
	DdsAddVariableV(p, &EXPY, "EXPY", 0            , 0.0, CompExpY , 1, &y);
	DdsAddVariableV(p, &x, "x", DDS_FLAG_REQUIRED, 0.0, NULL, 0);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_REQUIRED, 0.0, NULL, 0);

	pVs = DdsVariables(&nv, p);
	for (i = 0; i < nv; ++i) {
		pVr = DdsRhsvs(&nr, pVs[i]);
		for (j = 0; j < nr; ++j) {
			pVr[j] = *((DDS_VARIABLE*)pVr[j]);
		}
	}
	DdsCompileGraph(p, 0);
	DdsComputeStatic(p);
	DdsDbgPrintF(stdout, "After DdsComputeStatic(p)", p);

	DdsDeleteProcessor(&p);
	getchar();
	return;
}

double CompY(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	DDS_VARIABLE x1 = DdsGetRHSV(y, 0);
	DDS_VARIABLE x2 = DdsGetRHSV(y, 1);
	double xv1 = DdsGetValue(x1);
	double xv2 = DdsGetValue(x2);
	return exp(xv1) - xv2;
}

int TestNL2()
{
	DDS_PROCESSOR p;
	DDS_VARIABLE y, x1, x2;
	DdsCreateProcessor(&p, 10);
	DdsAddVariableV(p, &x1, "x1", DDS_FLAG_REQUIRED, 1.0, NULL, 0); /* Register x1 to p. */
	DdsAddVariableV(p, &x2, "x2", DDS_FLAG_SET, 2.0, NULL, 0);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_TARGETED, 0.0, CompY, 2, x1, x2);
	DdsCompileGraph(p, 0); /* Check relations and determine computation order. */
	DdsComputeStatic(p);   /* Compute variable's value according to defined(by DdsCompileGraph()) order. */
	printf("Value: y=%lf x1=%lf x2=%lf\n", DdsGetValue(y), DdsGetValue(x1), DdsGetValue(x2));
	DdsDeleteProcessor(&p);
	getchar();
	return 0;
}

double CompDY2(DDS_PROCESSOR p, DDS_VARIABLE y)
{
	DDS_VARIABLE A = DdsGetRHSV(y, 0);
	DDS_VARIABLE B = DdsGetRHSV(y, 1);
	DDS_VARIABLE C = DdsGetRHSV(y, 2);
	DDS_VARIABLE Y = DdsGetRHSV(y, 3);
	double AV = DdsGetValue(A);
	double BV = DdsGetValue(B);
	double CV = DdsGetValue(C);
	double yv = DdsGetValue(Y);
	return AV*(CV-yv)+BV;
}

int main()
{
	/* dy/dt = A(C-y) + B */
	/* dy/dt(inf) = (AC+B)/A */
	/* Steady state y = 5.0 */
	DDS_PROCESSOR p;
	DDS_VARIABLE y, dydt, A,B,C;
	DDS_VARIABLE time, step;
	DDS_VARIABLE* pVs, * pVr;
	int i,j,nv, nr;

	DdsCreateProcessor(&p, 10);
	DdsAddVariableV(p, &A, "A", DDS_FLAG_SET, 1.0, NULL, 0);
	DdsAddVariableV(p, &B, "B", DDS_FLAG_SET, 2.0, NULL, 0);
	DdsAddVariableV(p, &C, "C", DDS_FLAG_SET, 3.0, NULL, 0);
	DdsAddVariableV(p, &y, "y", DDS_FLAG_REQUIRED|DDS_FLAG_INTEGRATED, 1.0, NULL,1, &dydt);
	DdsAddVariableV(p, &dydt, "dydt", DDS_FLAG_REQUIRED, 0.0, CompDY2, 4,&A,&B,&C,&y);
	time = DdsTime(p);
	step = DdsStep(p);

	pVs = DdsVariables(&nv, p);
	for (i = 0; i < nv; ++i) {
		pVr = DdsRhsvs(&nr, pVs[i]);
		for (j = 0; j < nr; ++j) {
			pVr[j] = *((DDS_VARIABLE*)pVr[j]);
		}
	}

	DdsSetValue(step, 0.1);
	DdsCompileGraph(p, 0);
	for (i = 0; i < 100; ++i) {
		DdsComputeDynamic(p,0);
		printf("Time=%lf dydt=%lf y=%lf\n", DdsGetValue(time), DdsGetValue(dydt), DdsGetValue(y));
	}
	DdsCompileGraph(p, DDS_STEADY_STATE);
	DdsComputeStatic(p);
	printf("Time=%lf dydt=%lf y=%lf\n", DdsGetValue(time), DdsGetValue(dydt), DdsGetValue(y));

	DdsDeleteProcessor(&p);
	getchar();
}