all: alg_example SR_ctl 
#	-----------------------------------------------------------------
alg_example: alg_base.cpp alg_example.cpp
	gcc -c -fpic -shared -o alg_base.o alg_base.cpp			2>>err.txt
	gcc -c -fpic -o alg_example.o alg_example.cpp				2>>err.txt		
	gcc -shared	-o alg_example.so alg_example.o alg_base.o		2>>err.txt	
	rm -f *.o		
#	-----------------------------------------------------------------
SR_ctl: SR_ctl.cpp alg_base.cpp config_SR.cpp alg_base.h config_SR.h SR_ctl.h
	gcc -c -shared -o alg_base.o alg_base.cpp	2>>err.txt
	gcc -c -shared -o config_SR.o config_SR.cpp 2>>err.txt	
	mpic++ -std=c++0x -c -shared -o SR_ctl.o SR_ctl.cpp -lboost_mpi  	2>>err.txt
	mpic++ -o SR_ctl SR_ctl.o alg_base.o config_SR.o -lboost_mpi -ldl	2>>err.txt
	rm -f *.o	
