USE_STATIC := 1

ifeq ($(USE_STATIC), 1)
CRT1 := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/crt1.o
else
CRT1 := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/Scrt1.o
endif

# CRTn can be skipped after CRTend is skipped for sum
CRTn := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/crtn.o

# CRTend can be skipped for sum after CRTi and CRTbegin are skipped
ifeq ($(USE_STATIC), 1)
CRTend := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtend.o
else
CRTend := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtendS.o
endif

# CRTi can be skipped for sum
CRTi := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/crti.o

# CRTbegin can be skipped for sum
ifeq ($(USE_STATIC), 1)
CRTbegin := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtbeginT.o
else
CRTbegin := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtbeginS.o
endif

# LIBGCC can be skipped for sum
LIBGCC := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a

ifeq ($(USE_STATIC), 1)
LIBC := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a
else

# this is a linker script. We can expand the content of the linker script to the command line explicitly
# LIBC := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.so
LIBC := /lib/i386-linux-gnu/libc.so.6
endif

# don't see this used in dynamic linking case
LIBGCC_EH := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a

# all:

runld_myown_libc:
	rm -f ./a.out
	ld -melf_i386 -static $(CRT1) $(CRTi) artifact/sum.gas.o $(CRTn)
	./a.out

runld:
	rm -f ./a.out
ifeq ($(USE_STATIC), 1)
	ld -melf_i386 -static $(CRT1) $(CRTi) artifact/sum.gas.o --start-group $(LIBC) $(LIBGCC) $(LIBGCC_EH) --end-group $(CRTn)
else
	ld -melf_i386 -I /lib/ld-linux.so.2 $(CRT1) artifact/sum.gas.o $(LIBC) # ld use shared library by default
endif
	./a.out

LIB_SEARCH_PATH := -L/usr/lib/gcc/x86_64-linux-gnu/11/32 -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../../lib32 -L/lib/i386-linux-gnu -L/lib/../lib32 -L/usr/lib/i386-linux-gnu -L/usr/lib/../lib32 -L/usr/lib/gcc/x86_64-linux-gnu/11 -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/11/../../.. -L/lib/i386-linux-gnu -L/usr/lib/i386-linux-gnu

# the command simplified from collect2 command line called by the gcc driver
raw:
	rm -f ./a.out
ifeq ($(USE_STATIC), 1)
	ld -melf_i386 -static $(CRT1) $(CRTi) $(CRTbegin) $(LIB_SEARCH_PATH) artifact/sum.gas.o -lgcc -lgcc_eh -lc -lgcc -lgcc_eh -lc $(CRTend) $(CRTn)
else
	ld -melf_i386 -I /lib/ld-linux.so.2 $(CRT1) $(CRTi) $(CRTbegin) $(LIB_SEARCH_PATH) artifact/sum.gas.o -lgcc -lgcc_s -lc -lgcc -lgcc_s $(CRTend) $(CRTn)
endif
	./a.out

use_driver:
ifeq ($(USE_STATIC), 1)
	gcc -m32 -v -static artifact/sum.gas.o
else
	gcc -m32 -v artifact/sum.gas.o
endif
	./a.out

runnm:
	gcc -m32 -v artifact/sum.gas.o 2>&1 | grep collect2 | tee /tmp/collect2.cmd | tr ' ' '\n' | grep '\.o' | xargs nm
	cat /tmp/collect2.cmd

findlib:
ifdef LIBRE
	for dir in `gcc -m32 -v artifact/sum.gas.o 2>&1 | grep collect2 | tr ' ' '\n' | grep '\-L' | sed 's/-L//'`; do echo "== $$dir =="; ls $$dir | grep $$LIBRE || true; done
else
	echo "Must specify LIBRE"
endif
