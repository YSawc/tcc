#include "tcc.h"

static Var *lVars;
static Var *gVars;
static int conditional_c = 0;
static Scope *scope;

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *fn_nm;
void set_current_fn(char *c) { fn_nm = c; }

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

static Node *new_expr(Node *lhs) {
  Node *nd = new_nd(ND_EXPR);
  nd->lhs = lhs;
  return nd;
}

static Var *consume_contents(Var *v) {
  if (token->kind != TK_STR)
    error_at(token->str, "expect contents.");
  v->contents = token->str;
  v->len = token->len + 1;
  token = token->next;
  return v;
}

static Node *phase_typ_rev(void);
static Node *stmt(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *idx(void);
static Node *primary_expr(void);

static Node *expect_dec(Type *ty) {
  Token *tok = consume_ident();

  if (consume("[")) {
    int l = expect_number();
    expect(']');
    Var *v = new_arr_var(tok->str, l, ty);
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
    Var *v = new_l_var(consume_ident()->str, ty);
    v->ln = conditional_c++;
    v->ty->base = ty;
    if (consume("=")) {
      v = consume_contents(v);
    };
    return new_nd(ND_NULL);
  }

  Var *v = new_l_var(tok->str, ty);
  // In this case, it only declared but not assigned.
  if (startswith(token->str, ";"))
    return new_nd(ND_NULL);
  expect('=');
  Node *nd = new_binary(ND_ASSIGN, new_var_nd(v), add());

  return new_expr(nd);
}

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

static Type *look_ty(void) {
  if (!strcmp(token->str, "int"))
    return ty_int;
  if (!strcmp(token->str, "char")) {
    if (!strcmp(token->next->str, "*"))
      return ty_d_by;
    else
      return ty_char;
  }
  return NULL;
}

static void consume_ty(Type *ty) {
  // Detector in list of reserved multiple letter string
  if (ty == ty_char || ty == ty_int) {
    token = token->next;
  } else if (ty == ty_d_by) {
    token = token->next->next;
  }
}

static Type *expect_ty() {
  Type *ty = look_ty();
  if (!ty)
    error_at(token->str, "expected type.");
  consume_ty(ty);
  return ty;
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
    Type *ty = expect_ty();
    new_l_var(expect_ident(), ty);
    args_c++;
    while (consume(",")) {
      Type *ty = expect_ty();
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
  fn->lv = lVars;
  return fn;
}

// program = stmt*
Program *gen_program(void) {
  gVars = NULL;

  Function head_fn = {};
  Function *fn_cur = &head_fn;

  // detect global functoin.
  while (!is_fn()) {
    Type *ty = expect_ty();
    if (!ty)
      error_at(token->str, "expected type, but not detected.");
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
  prog->gv = gVars;
  return prog;
}

void assign_var_offset(Function *fn) {
  int i = 0;
  for (Var *v = fn->lv; v; v = v->next) {
    // data byte not assigned offset but labeled in sectio of data.
    i += v->ty->size;
    v->offset = i;
  }
}

// emit rsp counts of variables in fn.
void emit_rsp(Function *func) {
  int c = 0;

  // shift rsp counter counts of local variable.
  for (Var *v = func->lv; v; v = v->next)
    c += v->ty->size;

  // sub rsp is multiples of 8. So rsp need roud up with 8 as nardinality.
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
    nd->cond = equality();
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

  Type *ty = look_ty();
  if (ty) {
    consume_ty(ty);
    Node *nd = expect_dec(ty);
    expect(';');
    return nd;
  }

  Node *nd = new_expr(assign());
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
    return new_binary(ND_MUL, nd, mul());
  } else if (consume("/")) {
    return new_binary(ND_DIV, nd, mul());
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
  } else {
    return idx();
  }
}

/* idx = primary "[" num "]" */
static Node *idx() {
  Node *nd = primary_expr();

  if (consume("[")) {
    Node *add_nd = new_add(nd, primary_expr());
    Node *nd = new_uarray(ND_REF, add_nd);
    expect(']');
    return nd;
  }

  return nd;
}

/* primary_expr = NUM */
/*              | "(" "{" (stmt)* "}" ")" */
/*              | Ident ("()")? */
/*              | Ident */
/*              | ImmString "[" num "]" */
/*              | "(" add ")" ) */
/*              | "sizeof" "(" add ")" */
/*              | "\"" {contents} "\"" */
static Node *primary_expr() {
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
      if (cur->kind != ND_EXPR)
        error("stmt expr returning void is not supported");
      memcpy(cur, cur->lhs, sizeof(Node));
      Node *nd = new_nd(ND_GNU_BLOCK);
      nd->block = head.next;
      return nd;
    } else {
      Node *nd = add();
      expect(')');
      return nd;
    }
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
    if (!v)
      error_at(token->str, "can't handle with undefined variable.");
    if (v->ty == ty_d_by) {
      if (consume("=")) {
        v = consume_contents(v);
        return new_binary(ND_ASSIGN, new_var_nd(v), new_nd(ND_NULL));
      }
    }
    return new_var_nd(v);
  }

  if (consume("sizeof")) {
    Node *nd = calloc(1, sizeof(Node));
    expect('(');
    Token *tok = consume_ident();
    if (!tok)
      error_at(token->str, "expected ident.");
    Var *v = find_var(tok);
    if (!v)
      error_at(token->str, "expected declaration variable.");
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
    v->next = lVars;
    v->ty = ty_d_by;
    v->ty->base = ty_d_by;
    v->len = token->len + 1;
    v->contents = token->str;
    v->ln = conditional_c++;
    lVars = v;
    token = token->next;
    return new_var_nd(v);
  }

  return new_num(expect_number());
}
