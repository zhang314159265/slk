SLIBC_SRC := $(wildcard *.c)
SLIBC_OBJ := $(patsubst %.c,out/%.o,$(SLIBC_SRC))

# -fno-pic to avoid generating relocation for GOT/PLT
CFLAGS := -m32 -Werror -Wall -fno-pic

slibc.a: $(SLIBC_OBJ)
	ar r slibc.a $(SLIBC_OBJ)

out/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm slibc.a
	rm -rf out
