#include "tcc.h"

Node *nd;

static void emit_prologue() { printf("  .intel_syntax noprefix\n"); }

static void emit_fn(Function *fn) {
  char *fn_nm = fn->nm;
  printf("  .global %s\n", fn_nm);
  printf("%s:\n", fn_nm);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
}

static void emit_data(void) { printf("  .data\n"); }
static void emit_text(void) { printf("  .text\n"); }

static void emit_globals_data(Program *prog) {
  printf("  .data\n");
  for (Var *var = prog->gv; var; var = var->next) {
    printf("%s:\n", var->nm);

    if (!var->contents) {
      printf("  .zero %d\n", var->ty->size);
      continue;
    }

    for (int i = 0; i < strlen(var->contents); i++)
      printf("  .byte %d\n", var->contents[i]);
  }
}

static void emit_out_fn(Function *fn) {
  printf(".L.return.%s:\n", fn->nm);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void emit() {
  token = tokenize();
  Program *prog = gen_program();

  if (prog->gv)
    emit_globals_data(prog);

  // Variable created this phase, so assign each variable with offset.
  assign_var_offset(prog->fn);

  for (Function *fn = prog->fn; fn; fn = fn->next) {
    emit_data();
    for (Var *v = fn->lv; v; v = v->next) {
      if (v->ty == ty_d_by) {
        printf(".L.data.%d:\n", v->ln);
        for (int i = 0; i < strlen(v->contents); i++) {
          printf("  .byte %d\n", v->contents[i]);
        }
      }
    }

    emit_text();

    int o = 0; // offset for variables.
    // emit offset to each variables count up within starts from args data.
    for (Var *v = fn->lv; v; v = v->next) {
      o += v->ty->size;
      v->offset = o;
    }

    emit_fn(fn);
    emit_rsp(fn);
    emit_args(fn);
    set_current_fn(fn->nm);
    for (Node *nd = fn->nd; nd; nd = nd->next) {
      code_gen(nd);
    }

    emit_out_fn(fn);
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
