# /tmp/min size 4448 bytes
# after strip: 4248 bytes
# the effective size of the elf fize is very small. The main reason it's around 4K
# is because the text section is aligned at 4K file offset!
use_asm:
	gcc -m32 -o /tmp/min.o -c min.s
	ld -melf_i386 -static /tmp/min.o -o /tmp/min
	strip /tmp/min
	ls -l /tmp/min
	xxd -g 1 /tmp/min | grep -v '00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00'
	/tmp/min || [ $$? -eq 3 ]
	echo Pass

# /tmp/hello size 14KB.
use_c:
	gcc -m32 hello.c -o /tmp/hello
	ls -l /tmp/hello
	/tmp/hello
