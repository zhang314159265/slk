all:
	gcc -m32 artifact/sum.gas.o -m32
	./a.out
