#include "tcc.h"

Node *node;

static void emit_prologue() {
  printf("  .intel_syntax noprefix\n");
}

static void emit_function(Function *function) {
  printf("  .text\n");
  printf("  .global main\n");
  printf("%s:\n", function->name);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
}

static void emit_globals_data(Program *prog) {
  printf("  .data\n");
  for (Var *var = prog->gVars; var; var = var->next) {
    printf("%s:\n", var->name);
    printf("  .zero %d\n", var->type.type_size);
  }

}

static void emit_out_function(Function *function) {
  printf(".L.return.%s:\n", function->name);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void emit_data() {
  token = tokenize();
  Program *prog = gen_program();

  if (prog->gVars)
    emit_globals_data(prog);
  emit_function(prog->func);

  int i = 0;
  for (Var *v = prog->func->lVars; v; v = v->next) {
    i++;
    v->offset = i * 8;
  }

  // Variable created this phase, so assign each variable with offset.
  assign_var_offset(prog->func);
  emit_rsp(prog->func);

  for (Node *node = prog->func->node; node; node = node->next) {
    code_gen(node);
  }

  emit_out_function(prog->func);
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
