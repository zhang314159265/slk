#include <stdio.h>
#include "elf_writer.h"
#include "util.h"
#include <assert.h>

void test_hand_crafted_file() {
  #if 0
  asm:
    mov $1, %eax
    mov $2, %ebx
    int $0x80
  machine code:
    b8 01 00 00 00
    bb 02 00 00 00
    cd 80
  #endif
  struct str textstr = str_create(16);
  str_append(&textstr, 0xb8); str_append(&textstr, 0x01); str_append(&textstr, 0x00); str_append(&textstr, 0x00); str_append(&textstr, 0x00);
  str_append(&textstr, 0xbb); str_append(&textstr, 37); str_append(&textstr, 0x00); str_append(&textstr, 0x00); str_append(&textstr, 0x00);
  str_append(&textstr, 0xcd); str_append(&textstr, 0x80);

  const char* path = "/tmp/manual.elf";
  struct elf_writer writer = elf_writer_create();
  elf_writer_set_text_manually(&writer, &textstr);
  elf_writer_write(&writer, path);
  elf_writer_free(&writer);
  str_free(&textstr);
  assert(false && "execute manual.elf in the test!");

  // TODO: execute the manual.elf
}

int main(int argc, char **argv) {
  test_hand_crafted_file();
  return 0;
}
