#include "tcc.h"

Node *node;

static void emit_prologue() {
  printf("  .intel_syntax noprefix\n");
  printf("  .global main\n");
}

static void emit_global() {
  printf("main:\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");

  token = tokenize();

  Function *function = gen_program();

  int i = 0;
  for (Var *v = function->lVars; v; v = v->next) {
    i++;
    v->offset = i * 8;
  }

  // Variable created this phase, so assign each variable with offset.
  assign_var_offset(function);
  emit_rsp(function);

  for (Node *node = function->node; node; node = node->next) {
    code_gen(node);
  }
}

static void emit_data() { emit_global(); }

static void emit_end() {
  printf(".L.return:\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void gen_code() {
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
