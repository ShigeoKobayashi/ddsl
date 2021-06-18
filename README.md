# ddsl
DDSL(Digital Dynamic Simulation Library for C/C++,based on Graph Theory) is the library to assist numerical simulation for dynamics and statics.
DDSL implements Newton-method for solving non-linear algebraic eqution systems for static simulation and Euler,Backward Euler,and Runge-Kutta methods for solving ordinary differential equation systems for dynamic simulation.The users,although,need not use these methos directly with the assistance of Graph-theory.DDSL automaticaly determines computation sequence,constructs non-linear equation and ordinary differential equation systems if necessary,and solve them.

This software has been tested on Windows-10(32-bit & 64-bit) and Linux(64-bit CentOS-7).
To build this software on Linux => see makefile
To build this software on Windows => create Visual studio solution,and add source files.
To use this library => include ddsl.h in your source codes(see test.c) and link appropriate library files.
