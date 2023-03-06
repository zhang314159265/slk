CFLAGS := -Werror=return-type

all: libc

build_sar:
	gcc sar.c elf_member.c -I.. -g $(CFLAGS)

libc: build_sar
	# ./a.out /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a -s printf
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/../../../i386-linux-gnu/libc.a

# TODO: parse fail
libgcc: build_sar
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc.a

libgcc_eh: build_sar
	./a.out /usr/lib/gcc/x86_64-linux-gnu/11/32/libgcc_eh.a
