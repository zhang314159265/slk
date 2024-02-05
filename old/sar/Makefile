CFLAGS := -Werror

all: libc

build_sar:
	gcc sar.c elf_member.c -I.. -I../../third_party/scom/include -g $(CFLAGS) -m32

libc: build_sar
	# libgcc needs writev,syscall,__errno_location,sigemptyset,sigfillset,sigdelset,__fprintf_chk
	# libgcc_eh needs dl_iterate_phdr
	# crt1.o defines _dl_relocate_static_pie
	# crti.o defines _init,_fini
	# libgcc_eh defines _Unwind_Resume,__gcc_personality_v0
	# libgcc defines __udivdi3,__ctzdi2,__letf2,__unordtf2
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a -s printf,writev,syscall,__errno_location,sigemptyset,sigfillset,sigdelset,dl_iterate_phdr,__fprintf_chk -d _Unwind_Resume,__gcc_personality_v0,_init,_fini,__udivdi3,__ctzdi2,__letf2,__unordtf2,_dl_relocate_static_pie
	# ./a.out /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a

# TODO: parse fail
libgcc: build_sar
	# ./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a -s __udivdi3,__ctzdi2,__letf2,__unordtf2

libgcc_eh: build_sar
	# ./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a -s _Unwind_Resume,__gcc_personality_v0 -d strlen,dl_iterate_phdr,free,malloc,abort
