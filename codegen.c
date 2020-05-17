#include "tcc.h"

static Var *lVars;
static Var *gVars;
static int conditional_c = 0;
static Scope *scope;

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static char *fn_nm;
void set_current_fn(char *c) { fn_nm = c; }

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *nd) {
  Var *v = nd->v;
  if (nd->kind == ND_REF) {
    code_gen(nd->lhs);
  } else if (v->contents) {
    printf("  push offset .L.data.%d\n", v->ln);
  } else if (v->is_local) {
    printf("  lea rax, [rbp-%d]\n", v->offset);
    printf("  push rax\n");
  } else {
    printf("  push offset %s\n", v->nm);
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

static char *new_label(void) {
  int t = 20;
  char buf[t];
  sprintf(buf, ".L.data.%d", conditional_c++);
  return strndup(buf, t);
}

static void give_scope(Var *v) {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->v = v;
  sc->next = scope;
  scope = sc;
}

static Var *new_l_var(char *nm, Type *ty) {
  if (!ty)
    error_at(token->str, "expected type, but not detected.");

  Var *v = calloc(1, sizeof(Var));
  v->next = lVars;
  v->ty = ty;

  v->nm = nm;
  v->is_local = true;
  lVars = v;

  give_scope(v);
  return v;
}

static void new_g_var(Type *ty) {
  char *nm = expect_ident();
  expect(';');
  Var *v = calloc(1, sizeof(Var));
  v->next = gVars;
  v->ty = ty;
  v->nm = nm;
  give_scope(v);
  gVars = v;
}

static Var *new_arr_var(char *nm, int l, Type *ty) {
  Var *ret_v;
  Var *v = calloc(1, sizeof(Var));
  v->next = lVars;
  if (ty == ty_char)
    v->ty = ty_char_arr;
  else if (ty == ty_int)
    v->ty = ty_int_arr;
  else
    error_at(token->str, "not array type detected.");
  v->nm = nm;
  v->is_local = true;
  lVars = v;

  for (int l_i = 0; l_i < l; l_i++) {
    Var *tmp = calloc(1, sizeof(Var));
    tmp->next = lVars;
    tmp->ty = v->ty;
    tmp->nm = "";
    tmp->is_local = true;
    lVars = tmp;

    if (l_i == l - 1)
      ret_v = tmp;
  }

  give_scope(v);
  return ret_v;
}

// find variable scope of a whole
static Var *find_var(Token *tok) {
  for (Scope *s = scope; s; s = s->next) {
    Var *v = s->v;
    if (strlen(v->nm) == tok->len && !strncmp(v->nm, tok->str, tok->len))
      return v;
  }
  return NULL;
}

static Node *new_nd(NodeKind kind) {
  Node *nd = calloc(1, sizeof(Node));
  nd->kind = kind;
  return nd;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *nd = new_nd(kind);
  nd->lhs = lhs;
  nd->rhs = rhs;
  return nd;
}

static Node *new_uarray(NodeKind kind, Node *lhs) {
  Node *nd = new_nd(kind);
  nd->lhs = lhs;
  return nd;
}

static Node *new_num(long val) {
  Node *nd = new_nd(ND_NUM);
  nd->val = val;
  return nd;
}

static Node *new_var_nd(Var *v) {
  Node *nd = new_nd(ND_VAR);
  nd->v = v;
  nd->ty = v->ty;
  nd->str = v->nm;

  if (v->ty->base)
    nd->ty->base = v->ty->base;
  return nd;
}

static Node *expect_dec(Type *ty) {
  Token *tok = consume_ident();
  Var *v = find_var(tok);
  if (v)
    error_at(tok->str, "multiple declaration.");

  if (consume("[")) {
    int l = expect_number();
    expect(']');
    v = new_arr_var(tok->str, l, ty);
    v->ty->base = ty;
    Node *nd = new_nd(ND_NULL);
    nd->v = v;
    return nd;
  }

  if (consume("*")) {
    if (ty == ty_char)
      ty = ty_d_by;
    else
      error_at(token->str, "now not support initialize of reference of other "
                           "than char literal.");
    v = new_l_var(consume_ident()->str, ty);
    expect('=');
    v->contents = token->str;
    token = token->next;
    v->ln = conditional_c++;
    v->ty->base = ty;
    return new_nd(ND_NULL);
  }

  v = new_l_var(tok->str, ty);
  // In this case, it only declared but not assigned.
  if (startswith(token->str, ";"))
    return new_nd(ND_NULL);
  return new_var_nd(v);
}

static Node *phase_typ_rev(void);
static Node *stmt(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary_expr(void);

static Node *func_args() {
  if (consume(")"))
    return NULL;

  Node *head = add();
  Node *cur = head;
  while (consume(",")) {
    cur->next = add();
    cur = cur->next;
  }
  expect(')');
  return head;
}

static bool is_fn(void) {
  Token *tok = token;
  expect_str("int");
  bool isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
}

static Type *look_type(void) {
  if (strcmp(token->str, "int") == 0)
    return ty_int;
  if (strcmp(token->str, "char") == 0)
    return ty_char;
  return NULL;
}

static void consume_typ(void) {
  // list of reserved multiple letter.string
  static char *types[] = {"int", "char"};

  // Detector in list of reserved multiple letter string
  for (int i = 0; i < sizeof(types) / sizeof(*types); i++) {
    if (startswith(token->str, types[i])) {
      token = token->next;
      break;
    }
  }
}

static Function *fn() {
  lVars = NULL;

  int args_c = 0;
  Function *fn = calloc(1, sizeof(Function));
  Node head_nd = {};
  Node *cur_nd = &head_nd;

  // in this scope, be in internal of fn.
  expect_str("int");
  fn_nm = expect_ident();
  expect('(');
  if (!consume(")")) {
    Type *ty = look_type();
    consume_typ();
    new_l_var(expect_ident(), ty);
    args_c++;
    while (consume(",")) {
      Type *ty = look_type();
      consume_typ();
      new_l_var(expect_ident(), ty);
      args_c++;
    }
    expect(')');
  }
  expect('{');
  while (!consume("}")) {
    cur_nd->next = phase_typ_rev();
    cur_nd = cur_nd->next;
  }

  fn->nm = fn_nm;
  fn->nd = head_nd.next;
  fn->args_c = args_c;
  fn->lv= lVars;
  return fn;
}

// program = stmt*
Program *gen_program(void) {
  gVars = NULL;

  Function head_fn = {};
  Function *fn_cur = &head_fn;

  // detect global functoin.
  while (!is_fn()) {
    Type *ty = look_type();
    if (!ty)
      error_at(token->str, "expected type, but not detected.");
    consume_typ();
    new_g_var(ty);
  }

  while (!at_eof()) {
    Scope *sc = scope;
    fn_cur->next = fn();
    fn_cur = fn_cur->next;
    scope = sc;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->fn = head_fn.next;
  prog->gv= gVars;
  return prog;
}

void assign_var_offset(Function *fn) {
  int i = 0;
  for (Var *v = fn->lv; v; v = v->next) {
    // data byte not assigned offset but labeled in sectio of data.
    if (v->ty != ty_d_by) {
      i += v->ty->size;
      v->offset = i;
    }
  }
}

// emit rsp counts of variables in fn.
void emit_rsp(Function *func) {
  int c = 0;

  // shift rsp counter counts of local variable.
  for (Var *v = func->lv; v; v = v->next)
    if (v->ty != ty_d_by)
      c += v->ty->size;

  // rsp is multiples of 8. So rsp need roud up with 8 as nardinality.
  c = ((c + 8 - 1) / 8) * 8;
  printf("  sub rsp, %d\n", c);
}

void emit_args(Function *fn) {
  if (!fn->lv)
    return;

  // Push arguments to the stack
  int i = 0;
  Var *v = fn->lv;
  for (int c = 0; c < fn->args_c; c++) {
    printf("  mov [rbp-%d], %s\n", v->offset, arg_regs[fn->args_c - i - 1]);
    v = v->next;
    i++;
  }
}

// In this phase, reveal type of each returned node.
static Node *phase_typ_rev() {
  Node *nd = stmt();
  typ_rev(nd);
  return nd;
}

/* stmt = "return" expr ";" */
/*      | "if" "(" cond ")" stmt ("else" stmt)?  */
/*      | "while" "(" cond ")" stmt ";" */
/*      | "{" stmt* "}" */
static Node *stmt() {
  if (consume("return")) {
    Node *nd = new_uarray(ND_RETURN, assign());
    expect(';');
    return nd;
  }

  if (consume("if")) {
    Node *nd = new_nd(ND_IF);
    nd->ln = conditional_c++;
    expect('(');
    nd->cond = relational();
    expect(')');
    nd->then = stmt();
    if (consume("else")) {
      nd->els = stmt();
    }
    return nd;
  }

  if (consume("while")) {
    Node *nd = new_nd(ND_WHILE);
    nd->ln = conditional_c++;
    expect('(');
    nd->cond = equality();
    expect(')');
    nd->stmt = stmt();
    return nd;
  }

  if (consume("{")) {
    Node head = {};
    Node *cur = &head;

    Scope *sc = scope;
    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }
    scope = sc;

    Node *nd = new_nd(ND_BLOCK);
    nd->block = head.next;
    return nd;
  }

  Node *nd = assign();
  expect(';');
  return nd;
}

// assign = equality ("=" assign)?
static Node *assign() {
  Node *nd = equality();

  if (consume("=")) {
    Node *n = new_binary(ND_ASSIGN, nd, add());
    return n;
  }
  return nd;
}

/* equality = relational ("==" relational | "!=" relational )* */
static Node *equality(void) {
  Node *nd = relational();

  for (;;) {
    if (consume("==")) {
      nd = new_binary(ND_EQ, nd, relational());
    } else if (consume("!=")) {
      nd = new_binary(ND_NEQ, nd, relational());
    } else {
      return nd;
    }
  }
};
/* relational = add ("<" add | "<=" add | ">" add | ">=" add)* */

static Node *relational(void) {
  Node *nd = add();
  for (;;) {
    if (consume("<")) {
      nd = new_binary(ND_LT, nd, add());
    } else if (consume("<=")) {
      nd = new_binary(ND_LTE, nd, add());
    } else if (consume(">")) {
      nd = new_binary(ND_LT, add(), nd);
    } else if (consume(">=")) {
      nd = new_binary(ND_LTE, add(), nd);
    } else {
      return nd;
    }
  }
};

static Node *new_add(Node *lhs, Node *rhs) {
  typ_rev(lhs);
  typ_rev(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_ADD, lhs, rhs);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_ADD, lhs, rhs);
  if (is_integer(lhs->ty) && rhs->ty->base)
    return new_binary(ND_PTR_ADD, rhs, lhs);
  error_at(token->str, "invalid operands");
  return NULL;
}

static Node *new_sub(Node *lhs, Node *rhs) {
  typ_rev(lhs);
  typ_rev(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_SUB, lhs, rhs);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_SUB, lhs, rhs);
  if (is_integer(lhs->ty) && rhs->ty->base)
    return new_binary(ND_PTR_SUB, rhs, lhs);
  error_at(token->str, "invalid operands");
  return NULL;
}

/* add = mul ( "*" mul | "/" mul )* */
static Node *add() {
  Node *nd = mul();

  if (consume("+")) {
    nd = new_add(nd, add());
  } else if (consume("-")) {
    nd = new_sub(nd, add());
  }

  return nd;
}

/* mul = unary ( "*" unary | "/" unary )* */
static Node *mul() {
  Node *nd = unary();

  if (consume("*")) {
    nd = new_binary(ND_MUL, nd, mul());
  } else if (consume("/")) {
    nd = new_binary(ND_DIV, nd, mul());
  }

  return nd;
}

/* unary = ( "+" | "-" | "*" | "&" )? primary_expr */
static Node *unary() {
  if (consume("+")) {
    return unary();
  } else if (consume("-")) {
    return new_binary(ND_SUB, new_num(0), unary());
  } else if (consume("*")) {
    return new_uarray(ND_REF, unary());
  } else if (consume("&")) {
    return new_uarray(ND_ADDR, unary());
    return nd;
  } else {
    return primary_expr();
  }
}

/* primary_expr = NUM */
/*              | "(" "{" (stmt)* "}" ")" */
/*              | Ident ("()")? */
/*              | Ident */
/*              | Ident "[" num "]" */
/*              | "(" add ")" ) */
/*              | "sizeof" "(" add ")" */
/*              | "\"" {contents} "\"" */
static Node *primary_expr(void) {
  if (consume("(")) {
    if (consume("{")) {
      // read gnu statement.
      Node head = {};
      Node *cur = &head;

      Scope *sc = scope;
      while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
      }
      scope = sc;

      expect(')');
      Node *nd = new_nd(ND_GNU_BLOCK);
      nd->block = head.next;
      return nd;
    } else {
      nd = add();
      expect(')');
      return nd;
    }
  }

  Type *ty = look_type();
  if (ty) {
    consume_typ();
    return expect_dec(ty);
  }

  Token *tok = consume_ident();

  if (tok) {
    // find fn.
    if (consume("(")) {
      Node *nd = new_nd(ND_FNC);
      nd->ln = conditional_c++;
      nd->str = tok->str;
      nd->args = func_args();
      return nd;
    }

    // find variable.
    Var *v = find_var(tok);
    if (v) {
      if (consume("[")) {
        Node *ref_nd = new_var_nd(v);
        Node *add_nd = new_add(ref_nd, primary_expr());
        Node *uarray_nd = new_uarray(ND_REF, add_nd);
        expect(']');
        return uarray_nd;
      } else {
        return new_var_nd(v);
      }
    }
    error_at(token->str, "can't handle with undefined variable.");
  }

  if (consume("sizeof")) {
    Node *nd = calloc(1, sizeof(Node));
    expect('(');
    Token *tok = consume_ident();
    if (!tok)
      error_at(tok->str, "expected ident.");
    Var *v = find_var(tok);
    if (!v)
      error_at(tok->str, "expected declaration variable.");
    nd = new_num(v->ty->size);
    expect(')');
    return nd;
  }

  // case of char literal.
  if (consume("'")) {
    if (strlen(token->str) != 1)
      error_at(token->str, "contents type of char must be char.");
    int i = token->str[0];
    token = token->next;
    expect('\'');
    return new_num(i);
  }

  // case of string literal.
  if (token->kind == TK_STR) {

    // string should parsed as global variable because formed as data-section.
    Var *v = calloc(1, sizeof(Var));
    v->next = gVars;
    v->nm = new_label();
    v->ty = ty_char;
    v->contents = token->str;
    v->ln = conditional_c;
    gVars = v;
    token = token->next;
    Node *nd = new_var_nd(v);
    return nd;
  }

  return new_num(expect_number());
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
    if (nd->lhs->ty->kind == TY_INT_ARR || nd->lhs->ty->kind == TY_CHAR_ARR)
      printf("  add rsp, 8\n");
    return;
  case ND_ADDR:
    gen_var_addr(nd->lhs);
    return;
  case ND_REF:
    code_gen(nd->lhs);
    if (nd->ty->size == 1)
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
