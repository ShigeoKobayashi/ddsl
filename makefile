DLL_SOURCES=ddsl.cpp ft_check.cpp loop.cpp sieve.cpp utils.cpp
TEST_SOURCE=test.c
HEADERS=ddsl.h ddsl_main.h debug.h utils.h

lib:
	g++ -o libddsl.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -o test test.c -lddsl
run:
	LD_LIBRARY_PATH=. ./test
