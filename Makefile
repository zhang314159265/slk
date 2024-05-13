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
