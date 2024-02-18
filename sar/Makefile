ROOT := ..
CFLAGS := -I$(ROOT)/third_party/scom/include -I. -m32

LIBC_PATH := /usr/lib/i386-linux-gnu/libc.a

test: build_sar
	@echo "Testing sar"
	@make test_dump
	@make test_extract

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
