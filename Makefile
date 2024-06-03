first: runslk_slibc_a

slk:
	mkdir -p out
	gcc -m32 -g -Werror -I. -Ithird_party/scom/include slk.c -o out/slk

test:
	@make -C snm test
	@make -C sar test
	@make runslk_slibc_a runslk_slibc runld_slibc
	@echo ALL PASS

# setup sum for testing
# Add -fno-pic to avoid generating GOT/PLT related stuff.
setup_sum:
	mkdir -p out
	gcc tests/sum.c -m32 -c -fno-pic -o out/sum.gas.o


runslk_slibc_a: slk setup_sum
	make -C third_party/slibc
	out/slk third_party/slibc/out/slibc.a out/sum.gas.o
	chmod a+x a.out
	./a.out | grep 5050
	@echo Pass

runslk_slibc: slk setup_sum
	make -C third_party/slibc
	out/slk third_party/slibc/out/*.o out/sum.gas.o
	chmod a+x a.out
	./a.out | grep 5050
	@echo Pass

runld_slibc: setup_sum
	make -C third_party/slibc
	ld -melf_i386 third_party/slibc/out/*.o out/sum.gas.o -o a.out
	./a.out | grep 5050
	@echo Pass

run_with_driver: setup_sum
ifeq ($(USE_DYNAMIC), 1)
	echo "Do dynamic linking"
	gcc -m32 -v out/sum.gas.o
else
	echo "Do static linking"
	gcc -m32 -v -static out/sum.gas.o
endif
	./a.out

runnm: setup_sum
	gcc -m32 -v out/sum.gas.o 2>&1 | grep collect2 | tee /tmp/collect2.cmd | tr ' ' '\n' | grep '\.o' | xargs nm
	cat /tmp/collect2.cmd

# This works even if out/sum.gas.o is not created yet! We still get the full command calling collect2 even if it
# fail because out/sum.gas.o is not found.
LIB_SEARCH_PATHS := $(shell gcc -m32 -v out/sum.gas.o 2>&1 | grep collect2 | tr ' ' '\n' | grep '\-L' | sed 's/-L//')

findlib:
ifdef LIBRE
	@echo "Lib search paths are: $(LIB_SEARCH_PATHS)"
	for dir in $(LIB_SEARCH_PATHS); do echo "== $$dir =="; ls $$dir | grep $$LIBRE || true; done
else
	echo "Must specify LIBRE"
endif

include use_glibc.make
