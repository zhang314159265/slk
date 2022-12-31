CRT1 := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/Scrt1.o
CRTi := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/crti.o
CRTbegin := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtbeginS.o
CRTend := /usr/lib/gcc/x86_64-linux-gnu/11/32/crtendS.o
CRTn := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/crtn.o

LIB_SEARCH_PATH := -L/usr/lib/gcc/x86_64-linux-gnu/11/32 -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../../lib32 -L/lib/i386-linux-gnu -L/lib/../lib32 -L/usr/lib/i386-linux-gnu -L/usr/lib/../lib32 -L/usr/lib/gcc/x86_64-linux-gnu/11 -L/usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/11/../../.. -L/lib/i386-linux-gnu -L/usr/lib/i386-linux-gnu

LIBGCC := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a
LIGGCC_S := /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_s.so
LIBC := /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.so

runld:
	rm -f ./a.out
	ld -melf_i386 -I /lib/ld-linux.so.2 $(CRT1) $(CRTi) $(CRTbegin) artifact/sum.gas.o $(LIBGCC) $(LIBGCC_S) $(LIBC) $(CRTend) $(CRTn)
	./a.out

# the command simplified from collect2 command line called by the gcc driver
raw:
	rm -f ./a.out
	ld -melf_i386 -I /lib/ld-linux.so.2 $(CRT1) $(CRTi) $(CRTbegin) $(LIB_SEARCH_PATH) artifact/sum.gas.o -lgcc -lgcc_s -lc -lgcc -lgcc_s $(CRTend) $(CRTn)
	./a.out

use_driver:
	gcc -m32 artifact/sum.gas.o
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
