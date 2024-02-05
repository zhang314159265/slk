ROOT := ..
CFLAGS := -I$(ROOT)/third_party/scom/include -I. -m32

LIBC_PATH := /usr/lib/i386-linux-gnu/libc.a

test: build_sar
	@echo "Testing sar"
	@$(ROOT)/out/sar $(LIBC_PATH)

build_sar:
	gcc sar.c $(CFLAGS) -o $(ROOT)/out/sar
