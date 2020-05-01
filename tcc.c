#include "tcc.h"

Node *node;

static void emit_prologue() {
  printf("  .intel_syntax noprefix\n");
  printf("  .global main\n");
}

static void emit_in_function(Function *function) {
  printf("%s:\n", function->name);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
}

static void emit_out_function(Function *function) {
  printf(".L.return.%s:\n", function->name);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void emit_data() {
  token = tokenize();
  Function *function = gen_program();

  emit_in_function(function);

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

  emit_out_function(function);
}

static void gen_code() {
  emit_prologue();
  emit_data();
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
