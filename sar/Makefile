ROOT := ..
CFLAGS := -I$(ROOT)/third_party/scom/include -I. -m32 -g

LIBC_PATH := /usr/lib/i386-linux-gnu/libc.a
LIBGCC_PATH := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a
LIBGCC_EH_PATH := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a

test: build_sar
	@echo "Testing sar"
	@make test_dump
	@make test_extract
	@make test_resolve

build_sar:
	gcc sar.c $(CFLAGS) -o $(ROOT)/out/sar

# this assumes sar has been built
test_extract:
	@echo "Test extract"
	@$(ROOT)/out/sar $(LIBC_PATH) exit.o -o /tmp
	@readelf -h /tmp/exit.o | grep -q "ELF Header:"

test_dump:
	@echo "Test dump"
	@$(ROOT)/out/sar $(LIBC_PATH) | grep -q exit.o || echo "exit.o not found in dump"

test_resolve:
	@echo "Test resolve"
	@$(ROOT)/out/sar $(LIBC_PATH) -s printf -d _Unwind_Resume,__gcc_personality_v0,_init,_fini,__udivdi3,__ctzdi2,__letf2,__unordtf2
	@$(ROOT)/out/sar $(LIBGCC_PATH) -s __udivdi3
	@$(ROOT)/out/sar $(LIBGCC_EH_PATH) -s _Unwind_Resume -d strlen,dl_iterate_phdr,free,malloc,abort
