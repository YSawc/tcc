#include "tcc.h"

static void emit_prologue(void) { printf(".intel_syntax noprefix\n"); }

static void emit_fn(Function *fn) {
  char *fn_nm = fn->nm;
  printf(".global %s\n", fn_nm);
  printf("%s:\n", fn_nm);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
}

static void emit_bss(void) { printf(".bss\n"); }
static void emit_data(void) { printf(".data\n"); }
static void emit_text(void) { printf(".text\n"); }

static void emit_globals_data(Program *prog) {
  if (!prog->gv)
    return;
  for (Var *v = prog->gv; v; v = v->next) {
    if (v->ty == ty_int) {
      printf("%s:\n", v->nm);
      printf("  .zero %d\n", v->ty->size);
    } else if (v->contents) {
      printf(".L.data.%d:\n", v->ln);
      for (int i = 0; i < v->len; i++)
        printf("  .byte %d\n", v->contents[i]);
    }
  }
}

static void emit_out_fn(Function *fn) {
  printf(".L.return.%s:\n", fn->nm);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

static void emit(void) {
  token = tokenize();
  Program *prog = gen_program();

  emit_bss();
  emit_data();
  emit_globals_data(prog);
  emit_text();

  // Variable created this phase, so assign each variable with offset.
  for (Function *fn = prog->fn; fn; fn = fn->next) {
    int o = 0; // offset for variables.
    // emit offset to each variables count up within starts from args data.
    for (Var *v = fn->lv; v; v = v->next) {
      if (v->is_st) {
        for (Var *m = v->mem; m; m = m->next)
          o += m->ty->size;
        // struct itself is settled offset same as first member.
        v->offset = o;
      } else {
        // data byte not assigned offset but labeled in section of data.
        o += v->ty->size;
        v->offset = o;
      }
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

static void gen_code(void) {
  emit_prologue();
  emit();
}

// Returns the contents of a given file.
static char *read_file(char *path) {
  // Open and read the file.
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  int filemax = 10 * 1024 * 1024;
  char *buf = malloc(filemax);
  int size = fread(buf, 1, filemax - 2, fp);
  if (!feof(fp))
    error("%s: file too large", path);

  // Make sure that the string ends with "\n\0".
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  return buf;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }
  filename = argv[1];
  user_input = read_file(filename);

  gen_code();

  return 0;
}
