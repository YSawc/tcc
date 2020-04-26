#include "tcc.h"

void emit_prologue() {
  printf("  .intel_syntax noprefix\n");
  printf("  .global main\n");
}

void emit_global() {
  printf("main:\n");
  printf("  push rbp\n");

  printf("  mov rax, %d\n", atoi(user_input));

  printf("  mov rbp, rsp\n");
}

void emit_data() {
  emit_global();
}

void emit_end() {
  printf("  pop rbp\n");
  printf("  ret\n");
}

void gen_code() {
  emit_prologue();
  emit_data();
  emit_end();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }
  user_input = argv[1];

  gen_code();

  return 0;
}
