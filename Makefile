test:
	@make -C snm test
	@make -C sar test

# setup sum for testing
setup_sum:
	gcc tests/sum.c -m32 -c -o out/sum.gas.o
