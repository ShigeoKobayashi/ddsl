#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "ddsl.h"

/* defined in test1.c */
extern void Test1();
  extern void TestI1();
  extern void TestNL2();

/* defined in test2.c */
extern void Test2();
/* defined in test3.c */
extern void Test3();

int main(void)
{
	int c;
	do {
		printf("\nEnter test number or 'q'/'e':");
		do {c = getchar();} while (isspace(c));
		switch (c)
		{
		case '1': Test1(); break;
		case '2': Test2(); break;
		case '3': Test3(); break;
		default:break;
		}
	} while (c != 'e' && c != 'E' && c != 'q' && c != 'Q');
	return 0;
}

void PrintName(DDS_VARIABLE v)
{
	unsigned int f = DdsGetSystemFlag(v);
	if ((f & (DDS_SFLAG_FREE | DDS_FLAG_TARGETED)) != 0) {
		printf("%s", DdsGetVariableName(v));
		if ((f & DDS_SFLAG_FREE) != 0) printf("<F:");
		if ((f & DDS_FLAG_TARGETED) != 0) printf("<T:");
		printf("%d> ", DdsGetVariableScore(v)); /* Block number */
	}
	else {
		printf("%s ", DdsGetVariableName(v));
	}
}

void PrintInfo(DDS_PROCESSOR p)
{
	DDS_VARIABLE v = DdsGetVariableSequence(p, DDS_COMPUTED_ONCE);
	if (DdsGetExitStatus(p)->Status != 0) printf("The function called before this PrintInfo() failed, the following informations may be incorrect!\n");
	if (v != NULL) printf("\nDDS_COMPUTED_ONCE sequence:\n ");
	while (v != NULL) {
		PrintName(v);
		v = DdsGetVariableNext(v);
	}
	printf("\n");
	v = DdsGetVariableSequence(p, DDS_COMPUTED_EVERY_TIME);
	if (v != NULL) printf("\nDDS_COMPUTED_EVERY_TIME sequence:\n ");
	while (v != NULL) {
		PrintName(v);
		v = DdsGetVariableNext(v);
	}
	printf("\n");
	v = DdsGetVariableSequence(p, DDS_COMPUTED_ANY_TIME);
	if (v != NULL) printf("\nDDS_COMPUTED_ANY_TIME sequence:\n ");
	while (v != NULL) {
		PrintName(v);
		v = DdsGetVariableNext(v);
	}
	printf("\n");
	DdsDbgPrintF(stdout, "Variables Info", p);
}