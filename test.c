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
	DdsUserFlagOn(V[0], DDS_FLAG_REQUIRED);
	DdsCompileGraph(p);
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
		0 0 0 1 1 0 0 0  x5 y5         A2 0 0 1 0 0 1 0 0  x5 y2
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
	DdsAddVariableV(p, &Y[2], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[5], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[3], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[4], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[5], name, DDS_FLAG_TARGETED, 0.0, CompVal, 2, X[3], X[4]); name[1] += 1;
	DdsAddVariableV(p, &Y[6], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsAddVariableV(p, &Y[7], name, DDS_FLAG_TARGETED, 0.0, CompVal, 3, X[1], X[2], X[7]); name[1] += 1;
	DdsCompileGraph(p);
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
	DdsCompileGraph(p);
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
	char name[4];
	printf("=== Test2_2()---\n");
	DDS_PROCESSOR p;
	DdsCreateProcessor(&p, 5);
	name[0] = 'X';
	name[1] = '0';
	name[2] = 0;
	for (i = 0; i < 8; ++i) {
		name[0] = 'X';
		DdsAddVariableV(p, &X[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 0);
		name[0] = 'W';
		DdsAddVariableV(p, &W[i], name, DDS_FLAG_REQUIRED, 0.0, CompVal, 1,X[i]);
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
	DdsCompileGraph(p);
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
	DdsCompileGraph(p);
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
	DdsCompileGraph(p);
	DdsDeleteProcessor(&p);
	getchar();
}


void Test()
{
	Test2_2();
	return;
	Test1();
	Test2();
	Test2_1();
	Test2_2();
	Test3();
	Test3_1();
}

int main()
{
	Test();
}
