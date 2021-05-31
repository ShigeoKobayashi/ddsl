/*
 *
 * DDSL: Digital Dynamic Simulation Library (C/C++ Library).
 *
 * Copyright(C) 2020 by Shigeo Kobayashi(shigeo@tinyforest.jp).
 *
 */

#ifndef  __DDSL_H
#define  __DDSL_H 1
/*
 * Platform definition: BIT32,BIT64,WINDOWS,and LINUX.
 *   Folowing macros define BIT32,BIT64 and WINDOWS for Windows(VC),and LINUX for Linux(gcc).
 *   Other platforms(compilers) are not yet implemented.
 * 
 */
#ifdef _WIN32
  #define WINDOWS
  #define EXPORT(t)  __declspec(dllexport) t __stdcall
  #ifdef _WIN64
     /* 64bit Windows */
     #define BIT64
  #else
     /* 32bit Windows */
     #define BIT32
  #endif /* _WIN32 */
#else  /* ~_WIN32 */
  #define LINUX
  #define EXPORT(t) __attribute__((visibility ("default")))  t
  #ifdef __x86_64__
     /* 64bit Linux */
     #define BIT64
  #else
     /* 32bit Linux */
     #define BIT32
  #endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Function pointer to compute(and return) variable's value. */
/* Note:'void* p' == DDS_PROCESSOR, 'void* v===DDS_VARIABLE  */
typedef double (*ComputeVal)(void* p, void* v);

/*
 * DDSL inner structure (Direct access is not safe.)
 */
#ifdef __cplusplus
typedef struct VARIABLE {
#else
typedef struct {
#endif
	void*        UserPTR; /* just for user(DDSL never touch) */
	char*        Name;
	ComputeVal   Function;
	double       Value;
#ifdef __cplusplus
	VARIABLE**   Rhsvs;
#else
	void**       Rhsvs;
#endif
	unsigned int UFlag;
	unsigned int SFlag;
	int          Nr;
#ifdef __cplusplus
	VARIABLE* next;
#else
	void* next;
#endif
	int          index;
	int          score;
} DdsVariable;

#ifdef __cplusplus
typedef struct PROCESSOR {
#else
typedef struct {
#endif
		void*         UserPTR; /* just for user(DDSL never touch) */
		int           status;  /* status of function call */
		/* changed at DdsAddVariableV/A() stage */
		DdsVariable** Vars;    /* Variables registered  */
		int           v_count; /* Total number of variables registered to Vars[] */
		int           v_max;   /* Size of the Vars[] */

		/* Arrays dynamically allocated during the processing. */
		/* Allocated and changed by DdsCheckRouteFT() */
		DdsVariable**  Ts;      /* <T>s */
		int            t_count; /* number of <T>s effective */
		DdsVariable*** Fs;      /* Fs[i][0] is paired <F> with <T>[i],other Fs[i][] are <F>s connected. */
		int*           f_count; /* f_count[i] is the count of <F>s connected to <T>[i]. */
		int*           f_max;   /* f_max[i] is the max size of Fs[i][...]*/
		int           *b_size;  /* size of each block */
		int            b_count; /* total number of blocks. */
		DdsVariable**  Is;      /* <I>s */
		int            i_count; /* the size of Is[] */
		DdsVariable*** I_toDR;  /* route variables from Is[i] to <DR>s (used in Runge-Kutta method) */

		/* changed by DdsBuildSequence(): Sequence list */
		DdsVariable*  VTopOnceT;
		DdsVariable*  VEndOnceT;
		DdsVariable*  VTopAnyT;
		DdsVariable*  VEndAnyT;
		DdsVariable*  VTopEveryT;
		DdsVariable*  VEndEveryT;

		/* DdsComputeStatic() stage */
		unsigned int  stage;
		double*       jacobian;   /* Jacobian matrix (block base) */
		double*       delta;      /* the results of SolveJacobian() in  Newton method. (block base)*/
		double*       y_cur;      /* current value of <T>-value - value computed. (block base) */
		double*       y_next;     /* next value of <T>-value - value computed. (block base) */
		double*       x;          /* x(<F>s) before approximation(block base). */
		double*       dx;         /* dx of dy/dx (total base) */
		double*       scale;      /* scaling (used by LuDecomposition solving Jacobian) */
		int*          pivot;      /* pivot index  (used by LuDecomposition solving Jacobian) */
		double        eps;        /* convergence criteria */
} DdsProcessor;

/*
 *    Flags &  Functions ETC. 
 */
typedef DdsProcessor* DDS_PROCESSOR;  /* Processor handle */
typedef DdsVariable*  DDS_VARIABLE;   /* Variable handle  */

/*
 *  Variable flags(attributes) definition. 
 */
/*   User flags   */
#define DDS_FLAG_REQUIRED       0x00000001  /* <R> */
#define DDS_FLAG_SET            0x00000002  /* <S> */
#define DDS_FLAG_TARGETED       0x00000004  /* <T> */
#define DDS_FLAG_INTEGRATED     0x00000008  /* <I> */
#define DDS_FLAG_VOLATILE       0x00000010  /* <V> */
#define DDS_FLAG_DIVISIBLE      0x00000020  /* <D> */
#define DDS_FLAG_NON_DIVISIBLE  0x00000040  /* <N> */
#define DDS_FLAG_MASK           0x00000FFF  /* Mask */

/*   System flags   */
#define DDS_SFLAG_ALIVE         0x00001000  /* <AL> */
#define DDS_SFLAG_FREE          0x00002000  /* <FR> */
#define DDS_SFLAG_DIVIDED       0x00004000  /* <DD> */
#define DDS_SFLAG_DERIVATIVE    0x00008000  /* <DR> */
#define DDS_SFLAG_ERROR         0x00010000  /* <ER> (Variable having some contradicted flags.) */
#define DDS_COMPUTED_ONCE       0x00020000  /* <1T> (Variable computed once,conputed directly or indirectly by <S>.) */
#define DDS_COMPUTED_EVERY_TIME 0x00040000  /* <ET> (Variable computed at every integration steps,on upstream from <I> to <DE>.) */
#define DDS_COMPUTED_ANY_TIME   0x00080000  /* <AT> (Variable computed at any time,time dependent but not <E>.) */
#define DDS_SFLAG_MASK          0x00FFF000  /* Mask */

/* Processor flag */
#define DDS_SFLAG_CHECKED      0x01000000  /* Checked> */
#define DDS_SFLAG_STACKED      0x02000000  /* Stacked> */
#define DDS_COMPUTED_ALL       0x04000000  /* Evrything computed> */
#define DDS_PROC_MASK          0xFF000000  /* Mask */

/* Flag manipulation macros */
#define DDS_FLAG_OR(F,f)        (((F)&(f)) != 0)
#define DDS_FLAG_AND(F,f)       (((F)&(f)) == (f))
#define DDS_FLAG_ON(F,f)        ((F)|=(f))
#define DDS_FLAG_OFF(F,f)       ((F)&=(~(f)))

/* ERROR codes */
#define DDS_ERROR_MEMORY        -1  /* Memory allocation failure. */
#define DDS_MSG_MEMORY             "Memory allocation failure."

#define DDS_ERROR_REQUIRED      -2  /* No required variable. */
#define DDS_MSG_REQUIRED           "No required variable."

#define DDS_ERROR_NULL          -3  /* Null found in registerred variables or RHSV of a variable. */
#define DDS_MSG_NULL            "Null found in registerred variables or RHSV of a variable."

#define DDS_ERROR_FLAG          -4 /* Contradicted flag specified.*/
#define DDS_MSG_FLAG            "Contradicted flag specified."

#define DDS_ERROR_FT_NUMBER     -5 /* Total number of <F>s and <T>s are different in the same connected component. */
#define DDS_MSG_FT_NUMBER       "Total number of <F>s and <T>s are different in the same connected component. "

#define DDS_ERROR_FT_ROUTE      -6 /* Independent route can't find from each <F> to <T>. */
#define DDS_MSG_FT_ROUTE        "Independent route can't be found from each <F> to <T>. "

#define DDS_ERROR_INDEX         -7 /* Invalid RHSV index.*/
#define DDS_MSG_INDEX           "Invalid RHSV index."

#define DDS_ERROR_COMPUTED      -8 /* Nothing to be computed.*/
#define DDS_MSG_COMPUTED         "Nothing to be computed."

#define DDS_ERROR_CONVERGENCE   -9 /* Unable to obtain convergence for solving non-linear equation system.*/
#define DDS_MSG_CONVERGENCE     "Unable to obtain convergence for solving non-linear equation system."

#define DDS_ERROR_JACOBIAN      -10 /* Singular Jacobian matrix. */
#define DDS_MSG_JACOBIAN        "Singular Jacobian matrix."

#define DDS_ERROR_SYSTEM        -999 /* Undefined c/c++ level error. */
#define DDS_MSG_SYSTEM          "Undefined c/c++ level error."

EXPORT(int)           DdsCreateProcessor(DDS_PROCESSOR* p, int nv);
EXPORT(void)          DdsDeleteProcessor(DDS_PROCESSOR* p);
EXPORT(int)           DdsAddVariableV(DDS_PROCESSOR p,DDS_VARIABLE *pv,const char *name,unsigned int f,double val, ComputeVal func,int nr,...);
EXPORT(int)           DdsAddVariableA(DDS_PROCESSOR p, DDS_VARIABLE* pv, const char* name, unsigned int f, double val, ComputeVal func, int nr, DDS_VARIABLE** rhsvs);
EXPORT(int)           DdsCompileGraph(DDS_PROCESSOR p);
EXPORT(int)           DdsCheckVariable(DDS_VARIABLE hv);
EXPORT(int)           DdsSetRHSV(DDS_VARIABLE hv, int i, DDS_VARIABLE rv);
EXPORT(DDS_VARIABLE)  DdsGetRHSV(DDS_VARIABLE hv, int i, DDS_VARIABLE rv);

EXPORT(unsigned int)  DdsUserFlagOn(DDS_VARIABLE hv, unsigned int f);
EXPORT(unsigned int)  DdsUserFlagOff(DDS_VARIABLE hv, unsigned int f);
EXPORT(unsigned int)  DdsSystemFlag(DDS_VARIABLE hv);

EXPORT(void)          DdsDbgPrintF(FILE* f, const char* title, DDS_PROCESSOR p);

EXPORT(int)           DdsSieveVariable(DDS_PROCESSOR ph);
EXPORT(int)           DdsDivideLoop(DDS_PROCESSOR ph);
EXPORT(int)           DdsCheckRouteFT(DDS_PROCESSOR ph);
EXPORT(int)           DdsBuildSequence(DDS_PROCESSOR ph);

EXPORT(int)           DdsComputeStatic(DDS_PROCESSOR ph);

EXPORT(DDS_VARIABLE*) DdsVariables(int* nv, DDS_PROCESSOR p);
EXPORT(DDS_VARIABLE*) DdsRhsvs(int* nr, DDS_VARIABLE v);


#if defined(__cplusplus)
}  /* extern "C" { */
#endif
#endif /* __DDSL_H */
