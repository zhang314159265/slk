# LIBGCC can be skipped for sum
LIBGCC := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a

ifeq ($(USE_STATIC), 1)
LIBC := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a
else

# this is a linker script. We can expand the content of the linker script to the command line explicitly
# LIBC := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.so
LIBC := /lib/i386-linux-gnu/libc.so.6
endif

LIBGCC_EH := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a

# all:

MIN_LIBC_O := $(shell cat artifact/libc_min_obj_list)
MIN_LIBC_O := $(patsubst %, artifact/libc.o/%, $(MIN_LIBC_O))

MIN_LIBGCC_O := $(shell cat artifact/libgcc_min_obj_list)
MIN_LIBGCC_O := $(patsubst %, artifact/libgcc.o/%, $(MIN_LIBGCC_O))

MIN_LIBGCC_EH_O := $(shell cat artifact/libgcc_eh_min_obj_list)
MIN_LIBGCC_EH_O := $(patsubst %, artifact/libgcc_eh.o/%, $(MIN_LIBGCC_EH_O))

runld_min_obj_list:
	@rm -f ./a.out
	@ld -melf_i386 -static $(CRT1) $(CRTi) artifact/sum.gas.o $(MIN_LIBC_O) $(MIN_LIBGCC_O) $(MIN_LIBGCC_EH_O) $(CRTn)
	@./a.out
