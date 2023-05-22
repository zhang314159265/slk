#include <stdio.h>
#include "elf_writer.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>

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
  uint8_t child_exit_code = 52;
  struct str textstr = str_create(16);
  str_append(&textstr, 0xb8); str_append(&textstr, 0x01); str_append(&textstr, 0x00); str_append(&textstr, 0x00); str_append(&textstr, 0x00);
  str_append(&textstr, 0xbb); str_append(&textstr, child_exit_code); str_append(&textstr, 0x00); str_append(&textstr, 0x00); str_append(&textstr, 0x00);
  str_append(&textstr, 0xcd); str_append(&textstr, 0x80);

  const char* path = "/tmp/manual.elf";
  struct elf_writer writer = elf_writer_create();
  elf_writer_set_text_manually(&writer, &textstr);
  elf_writer_write(&writer, path);
  elf_writer_free(&writer);
  str_free(&textstr);

  int child_pid = fork();
  if (child_pid < 0) {
    assert(false && "fork fail");
  } else if (child_pid == 0) {
    // child process
    int rc;

    // chmod
    rc = chmod(path, 0700);
    assert(rc == 0 && "chmod fail");

    char* argv[] = {(char*) path, NULL};
    char* envp[] = {NULL};
    rc = execve(path, argv, envp); // no return on success
    printf("execve fail with error %s\n", strerror(errno));
    assert(false && "execve fail");
  }
  int child_status;
  int rc = waitpid(child_pid, &child_status, 0);
  assert(rc == child_pid);
  assert(WIFEXITED(child_status));
  int actual_exit_code = WEXITSTATUS(child_status);
  printf("The actual exit code is %d\n", actual_exit_code);
  assert(actual_exit_code == child_exit_code);
  printf("\033[32mSucceed!\033[0m\n");
}

int main(int argc, char **argv) {
  test_hand_crafted_file();
  return 0;
}
