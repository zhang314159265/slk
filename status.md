A linker implemented in C.

# NOTE
- Run `gcc -v` to see the detailed command called by the driver in each stage (preprocess/compile/assemble/linking)
- ld '-lLIBNAME' search for libLIBNAME.so before libLIBNAME.a . Incorrectly using libLIBNAME.a if libLIBNAME.so exists may cause a segfault.
- `ld --verbose` will show the default linker script; `ld -melf_i386 --verbase` to show the linker script for 32bit linking.

# Reference
- [collect2](https://gcc.gnu.org/onlinedocs/gccint/Collect2.html): collect2 is a wrapper to ld.
- [GCC Driver](http://web.cse.ohio-state.edu/~reeves.92/CSE2421au12/SlidesDay51.pdf): gcc is a driver to call preprocessor, compiler, assembler, and linker when needed.
- [ld document](https://sourceware.org/binutils/docs-2.39/ld.html)

# Scratch
- check how linker works when calling 'gcc' to build the sum executable.
  - how does ld create the executable based on sum.o and libraries
  - check how ar file works, understand the format..
