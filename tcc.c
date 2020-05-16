#include "tcc.h"

Node *node;

static void emit_prologue() { printf("  .intel_syntax noprefix\n"); }

static void emit_function(Function *function) {
  char *func_name = function->name;
  printf("  .global %s\n", func_name);
  printf("%s:\n", func_name);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
}

static void emit_data(void) { printf("  .data\n"); }
static void emit_text(void) { printf("  .text\n"); }

static void emit_globals_data(Program *prog) {
  printf("  .data\n");
  for (Var *var = prog->gVars; var; var = var->next) {
    printf("%s:\n", var->name);

    if (!var->contents) {
      printf("  .zero %d\n", var->typ->size);
      continue;
    }

    for (int i = 0; i < strlen(var->contents); i++)
      printf("  .byte %d\n", var->contents[i]);
  }
}

static void emit_out_function(Function *function) {
  printf(".L.return.%s:\n", function->name);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void emit() {
  token = tokenize();
  Program *prog = gen_program();

  if (prog->gVars)
    emit_globals_data(prog);

  // Variable created this phase, so assign each variable with offset.
  assign_var_offset(prog->func);

  for (Function *func = prog->func; func; func = func->next) {
    emit_data();
    for (Var *v = func->lVars; v; v = v->next) {
      if (v->typ == typ_d_by) {
        printf(".L.data.%d:\n", v->ln);
        for (int i = 0; i < strlen(v->contents); i++) {
          printf("  .byte %d\n", v->contents[i]);
        }
      }
    }

    emit_text();

    int o = 0; // offset for variables.
    // emit offset to each variables count up within starts from args data.
    for (Var *v = func->lVars; v; v = v->next) {
      o += v->typ->size;
      v->offset = o;
    }

    emit_function(func);
    emit_rsp(func);
    emit_args(func);
    set_current_func(func->name);
    for (Node *node = func->node; node; node = node->next) {
      code_gen(node);
    }

    emit_out_function(func);
  }
}

static void gen_code() {
  emit_prologue();
  emit();
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
