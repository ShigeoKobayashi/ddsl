DLL_SOURCES=ddsl.cpp ft_check.cpp loop.cpp sieve.cpp utils.cpp sequence.cpp solver.cpp integrator.cpp
TEST_SOURCES=test.c test1.c
HEADERS=ddsl.h ddsl_main.h debug.h utils.h

lib:
	g++ -std=c++1y -o libddsl.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -lm -o test $(TEST_SOURCES) -lddsl
run:
	LD_LIBRARY_PATH=. ./test
