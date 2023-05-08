CFLAGS := -I..

all:
	gcc -m32 test_elf_writer.c $(CFLAGS)
	rm /tmp/manual.elf
	./a.out || true
	chmod a+x /tmp/manual.elf
	/tmp/manual.elf
