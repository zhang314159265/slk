SLIBC_SRC := $(wildcard *.c)
SLIBC_OBJ := $(patsubst %.c,out/%.o,$(SLIBC_SRC))

CFLAGS := -m32 -Werror -Wall

slibc.a: $(SLIBC_OBJ)
	ar r slibc.a $(SLIBC_OBJ)

out/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm slibc.a
	rm -rf out
