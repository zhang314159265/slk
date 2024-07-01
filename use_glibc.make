CRT1 := /usr/lib/i386-linux-gnu/crt1.o
CRTi := /usr/lib/i386-linux-gnu/crti.o
CRTbegin := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtbeginT.o
CRTend := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtend.o
CRTn := /usr/lib/i386-linux-gnu/crtn.o

LIBGCC := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a
LIBGCC_EH := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a

ifneq ($(USE_DYNAMIC), 1)
LIBC := /usr/lib/i386-linux-gnu/libc.a
else
# this is a linker script. We can expand the content of the linker script to the command line explicitly
# LIBC := /usr/lib/i386-linux-gnu/libc.so
LIBC := /lib/i386-linux-gnu/libc.so.6
endif

LDFLAGS_GLIBC_PATHS := $(patsubst %, -L%, $(LIB_SEARCH_PATHS))

# Use '=' rather than ':=' since the object files may be extracted later
# when extract_libs action is run.
FULL_LIBC_O = $(wildcard out/libc_extract/*)
# skip artifact/libc.o/dl-reloc-static-pie.o since it defines _dl_relocate_static_pie which is already defined by CRT1
FULL_LIBC_O := $(filter-out out/libc_extract/dl-reloc-static-pie.o,$(FULL_LIBC_O))
# avoid redefine __memcpy_chk which is already defined by memcpy-ia32.o
FULL_LIBC_O := $(filter-out out/libc_extract/memcpy_chk-nonshared.o,$(FULL_LIBC_O))
# avoid redefine __memmove_chk which is already defined by memmove-ia32.o
FULL_LIBC_O := $(filter-out out/libc_extract/memmove_chk-nonshared.o,$(FULL_LIBC_O))
# avoid redefine __mempcpy_chk which is already defined by mempcpy-ia32.o
FULL_LIBC_O := $(filter-out out/libc_extract/mempcpy_chk-nonshared.o,$(FULL_LIBC_O))

FULL_LIBGCC_O = $(wildcard out/libgcc_extract/*)
FULL_LIBGCC_EH_O = $(wildcard out/libgcc_eh_extract/*)

AUTO_MIN_LIBC_O = $(shell cat out/libc_auto_min_obj_list)
AUTO_MIN_LIBC_O := $(patsubst %, out/libc_extract/%, $(AUTO_MIN_LIBC_O))

AUTO_MIN_LIBGCC_O = $(shell cat out/libgcc_auto_min_obj_list)
AUTO_MIN_LIBGCC_O := $(patsubst %, out/libgcc_extract/%, $(AUTO_MIN_LIBGCC_O))

AUTO_MIN_LIBGCC_EH_O = $(shell cat out/libgcc_eh_auto_min_obj_list)
AUTO_MIN_LIBGCC_EH_O := $(patsubst %, out/libgcc_eh_extract/%, $(AUTO_MIN_LIBGCC_EH_O))

extract_libs:
	mkdir -p out/libc_extract
	ar x $(LIBC) --output out/libc_extract
	mkdir -p out/libgcc_extract
	ar x $(LIBGCC) --output out/libgcc_extract
	mkdir -p out/libgcc_eh_extract
	ar x $(LIBGCC_EH) --output out/libgcc_eh_extract

setup_auto_min_obj_list:
	DUMP_NEEDED_OBJ=1 out/sar $(LIBC) -s printf,dl_iterate_phdr -d _Unwind_Resume,__gcc_personality_v0,_init,_fini,__udivdi3,__ctzdi2,__letf2,__unordtf2,_dl_relocate_static_pie
	mv /tmp/needed_obj out/libc_auto_min_obj_list
	DUMP_NEEDED_OBJ=1 out/sar $(LIBGCC) -s __udivdi3,__unordtf2,__letf2,__ctzdi2
	mv /tmp/needed_obj out/libgcc_auto_min_obj_list
	DUMP_NEEDED_OBJ=1 out/sar $(LIBGCC_EH) -s _Unwind_Resume,__gcc_personality_v0 -d strlen,dl_iterate_phdr,free,malloc,abort
	mv /tmp/needed_obj out/libgcc_eh_auto_min_obj_list

runld_glibc_auto_min_obj_list: setup_sum setup_auto_min_obj_list
	@rm -f ./a.out
	ld -melf_i386 -static $(CRT1) $(CRTi) out/sum.gas.o $(AUTO_MIN_LIBC_O) $(AUTO_MIN_LIBGCC_O) $(AUTO_MIN_LIBGCC_EH_O) $(CRTn)
	@./a.out

runld_glibc_full_flatten: setup_sum extract_libs
	@rm -f ./a.out
	@ld -melf_i386 -static $(CRT1) $(CRTi) out/sum.gas.o $(FULL_LIBC_O) $(FULL_LIBGCC_O) $(FULL_LIBGCC_EH_O) $(CRTn)
	@./a.out

# The command is simplified from the collect2 command called by the gcc driver
# It uses standalone library files (.a/.so) rather than individual extracted
# .o files.
runld_glibc_uselib: setup_sum
	rm -f ./a.out
ifeq ($(USE_DYNAMIC), 1)
	echo "dynamic linking"
	# XXX -lc is an easier way to add libc
	ld -melf_i386 -I /lib/ld-linux.so.2 $(CRT1) $(CRTi) $(CRTbegin) out/sum.gas.o $(LDFLAGS_GLIBC_PATHS) -lgcc -lgcc_s $(LIBC) $(CRTend) $(CRTn)
else
	echo "static linking"
	ld -melf_i386 -static $(CRT1) $(CRTi) $(CRTbegin) out/sum.gas.o $(LDFLAGS_GLIBC_PATHS) --start-group -lgcc -lgcc_eh -lc --end-group $(CRTend) $(CRTn)
endif
	./a.out
