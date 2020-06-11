#include "tcc.h"

/* static char *arg_regs[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"}; */
static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *nd) {
  Var *v = nd->v;
  if (nd->kind == ND_REF) {
    code_gen(nd->lhs);
  } else if (v->is_local) {
    printf("  lea rax, [rbp-%d]\n", v->offset);
    printf("  push rax\n");
  } else {
    if (v->ty == ty_int)
      printf("  push offset %s\n", v->nm);
    else if (v->ty == ty_char_arr)
      printf("  push offset .L.data.%d\n", v->ln);
  }
  return;
}

static void load_8(void) {
  printf("  pop rax\n");
  printf("  movsx rax, byte ptr [rax]\n");
  printf("  push rax\n");
}

static void load_32(void) {
  printf("  pop rax\n");
  printf("  movsxd rax, dword ptr [rax]\n");
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
  if (ty->kind == TY_CHAR_ARR || ty->kind == TY_CHAR)
    printf("  mov [rax], dil\n");
  else if (ty->kind == TY_B) {
    printf("  cmp rdi, 0\n");
    printf("  setne dil\n");
    printf("  movzb rdi, dil\n");
    printf("  mov [rax], dil\n");
  } else
    printf("  mov [rax], edi\n");
  printf("  push rdi\n");
}

static void postfix_gen(bool inc) {
  printf("  pop rax\n");
  if (inc)
    printf("  add rax, 1\n");
  else
    printf("  sub rax, 1\n");
  printf("  push rax\n");
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], edi\n");
  printf("  push rdi\n");
  printf("  pop rax\n");
  if (inc)
    printf("  sub rax, 1\n");
  else
    printf("  add rax, 1\n");
  printf("  push rax\n");
}

void code_gen(Node *nd) {
  switch (nd->kind) {
  case ND_NULL:
  case ND_VD:
    return;
  case ND_NUM:
    printf("  push %ld\n", nd->val);
    return;
  case ND_VAR:
    gen_var_addr(nd);
    if (nd->po)
      printf("  push [rsp]\n");
    if (nd->ty->kind == TY_B || nd->ty->kind == TY_CHAR)
      load_8();
    else if (nd->ty->kind != TY_INT_ARR && nd->ty->kind != TY_CHAR_ARR)
      load_32();
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
    if (nd->ty == ty_char || nd->ty == ty_char_arr || nd->ty == ty_d_by)
      load_8();
    else
      load_32();
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
  case ND_TIL:
    code_gen(nd->lhs);
    printf("  pop rax\n");
    printf("  not rax\n");
    printf("  push rax\n");
    return;
  case ND_SUR:
    code_gen(nd->lhs);
    printf("  push 0\n");
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return;
  case ND_INC:
    nd->lhs->po = 1;
    code_gen(nd->lhs);
    postfix_gen(1);
    return;
  case ND_DEC:
    nd->lhs->po = 1;
    code_gen(nd->lhs);
    postfix_gen(0);
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
      printf("  imul rdi, 4\n");
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
    if (lr_if_or(nd, TY_INT_ARR) || lr_if_or(nd, TY_D_BY))
      printf("  imul rdi, 1\n");
    else
      printf("  imul rdi, 4\n");
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
