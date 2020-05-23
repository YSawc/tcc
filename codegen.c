#include "tcc.h"

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *nd) {
  Var *v = nd->v;
  if (nd->kind == ND_REF) {
    code_gen(nd->lhs);
  } else if (v->is_local) {
    if (v->contents) {
      printf("  lea rax, [rbp-%d]\n", v->offset);
      printf("  push rax\n");
      printf("  push offset .L.data.%d\n", v->ln);
    } else {
      printf("  lea rax, [rbp-%d]\n", v->offset);
      printf("  push rax\n");
    }
  } else {
    if (v->ty == ty_int)
      printf("  push offset %s\n", v->nm);
    else if (v->ty == ty_d_by)
      printf("  push offset .L.data.%d\n", v->offset);
  }
  return;
}

static void load_8(void) {
  printf("  pop rax\n");
  printf("  movsx rax, byte ptr [rax]\n");
  printf("  push rax\n");
}

static void load_64(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

static void store_val(Type *ty) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (ty->kind == TY_CHAR_ARR)
    printf("  mov [rax], dil\n");
  else
    printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

void code_gen(Node *nd) {
  switch (nd->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf("  push %ld\n", nd->val);
    return;
  case ND_VAR:
    gen_var_addr(nd);
    if (nd->ty->kind != TY_INT_ARR && nd->ty->kind != TY_CHAR_ARR &&
        nd->ty->kind != TY_D_BY)
      load_64();
    return;
  case ND_ASSIGN:
    gen_var_addr(nd->lhs);
    code_gen(nd->rhs);
    store_val(nd->lhs->ty);
    return;
  case ND_ADDR:
    gen_var_addr(nd->lhs);
    return;
  case ND_REF:
    code_gen(nd->lhs);
    if (nd->ty == ty_char || nd->ty == ty_d_by)
      load_8();
    else
      load_64();
    return;
  case ND_IF: {
    int c = nd->ln;
    if (nd->els) {
      code_gen(nd->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.else.%d\n", c);
      code_gen(nd->then);
      printf("  jmp .L.end.%d\n", c);
      printf(".L.else.%d:\n", c);
      code_gen(nd->els);
      printf(".L.end.%d:\n", c);
    } else {
      code_gen(nd->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.end.%d\n", c);
      code_gen(nd->then);
      printf(".L.end.%d:\n", c);
    }
    return;
  }
  case ND_WHILE: {
    int c = nd->ln;
    printf(".L.begin.%d:\n", c);
    code_gen(nd->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.end.%d\n", c);
    code_gen(nd->stmt);
    printf("  jmp .L.begin.%d\n", c);
    printf(".L.end.%d:\n", c);
    return;
  }
  case ND_FNC: {
    int c = nd->ln;
    int args_c = 0;
    for (Node *arg = nd->args; arg; arg = arg->next) {
      code_gen(arg);
      args_c++;
    }

    for (int i = args_c - 1; i >= 0; i--)
      printf("  pop %s\n", arg_regs[i]);
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", c);
    printf("  mov rax, 0\n");
    printf("  call %s\n", nd->str);
    printf("  jmp .L.end.%d\n", c);
    printf(".L.call.%d:\n", c);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", nd->str);
    printf("  add rsp, 8\n");
    printf(".L.end.%d:\n", c);
    printf("  push rax\n");
    return;
  }
  case ND_GNU_BLOCK:
    for (Node *n = nd->block; n; n = n->next)
      code_gen(n);
    return;
  case ND_BLOCK:
    for (Node *n = nd->block; n; n = n->next)
      code_gen(n);
    return;
  case ND_EXPR:
    code_gen(nd->lhs);
    printf("  add rsp, 8\n");
    return;
  case ND_RETURN:
    code_gen(nd->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", fn_nm);
    return;
  default:;
  }

  code_gen(nd->lhs);
  code_gen(nd->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (nd->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_PTR_ADD:
    if (lr_if_or(nd, TY_CHAR_ARR) || lr_if_or(nd, TY_D_BY))
      printf("  imul rdi, 1\n");
    else
      printf("  imul rdi, 8\n");
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
    if (lr_if_or(nd, TY_INT_ARR) || lr_if_or(nd, TY_D_BY))
      printf("  imul rdi, 1\n");
    else
      printf("  imul rdi, 8\n");
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NEQ:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LTE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  default:
    error_at(nd->str, "can't generate code from this node.\n");
  }

  printf("  push rax\n");
}
