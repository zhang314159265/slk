ROOT := ..
CFLAGS := -I$(ROOT)/third_party/scom/include

snm:
	gcc snm.c $(CFLAGS) -o $(ROOT)/out/snm

test: snm
	make -C $(ROOT) setup_sum
	@echo "Testing snm"
	$(ROOT)/out/snm $(ROOT)/out/sum.gas.o | grep -q "printf"
