#include "tcc.h"

static Var *lVars;
static Var *gVars;
static Var *stVars;
static int conditional_c = 0;
static Scope *scope;

static char *arg_regs[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
/* static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"}; */

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

static Var *new_g_var(char *nm, Type *ty) {
  Var *v = calloc(1, sizeof(Var));
  v->next = gVars;
  v->ty = ty;
  v->nm = nm;
  gVars = v;

  give_scope(v);
  return v;
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

static Var *new_str(void) {
  Var *v = calloc(1, sizeof(Var));
  v->next = gVars;
  v->ty = ty_char_arr;
  v->ty->base = ty_d_by;
  v->len = token->len + 1;
  v->contents = token->str;
  v->ln = conditional_c++;
  gVars = v;
  token = token->next;
  return v;
}

// find variable scope of a whole
static Var *find_var(Token *tok) {
  for (Scope *s = scope; s; s = s->next) {
    Var *v = s->v;
    if (v->is_st)
      continue;
    if (strlen(v->nm) == tok->len && !strncmp(v->nm, tok->str, tok->len))
      return v;
  }
  return NULL;
}

static Var *find_st(Token *tok) {
  for (Scope *s = scope; s; s = s->next) {
    Var *v = s->v;
    if (!v->is_st)
      continue;
    if (strlen(v->nm) == tok->len && !strncmp(v->nm, tok->str, tok->len))
      return v;
  }
  return NULL;
}

// find variable scope in a struct
static Var *find_mem(Var *st, Token *tok) {
  for (Var *m = st->mem; m; m = m->next)
    if (strlen(m->nm) == tok->len && !strncmp(m->nm, tok->str, tok->len))
      return m;
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

static Node *phase_typ_rev(void);
static Node *stmt(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *postfix(void);
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

  if (ty == ty_d_by) {
    Var *v = new_l_var(tok->str, ty);
    v->ln = conditional_c++;
    if (ty == ty_d_by)
      v->ty->base = ty_char;
    else
      v->ty->base = ty;
    if (consume("=")) {
      Node *nd = new_binary(ND_ASSIGN, new_var_nd(v), primary_expr());
      return new_expr(nd);
    };
    return new_nd(ND_NULL);
  }

  Var *v = new_l_var(tok->str, ty);
  // In this case, it only declared but not assigned.
  if (consume("=")) {
    Node *nd = new_binary(ND_ASSIGN, new_var_nd(v), add());
    return new_expr(nd);
  }

  return new_nd(ND_NULL);
}

static Var *expect_init_v(Type *ty) {
  Token *tok = consume_ident();

  if (consume("*")) {
    if (ty == ty_char || ty == ty_vd)
      ty = ty_d_by;
    else
      error_at(token->str, "now not support initialize of reference of other "
                           "than char literal.");
    Var *v = new_l_var(expect_ident(), ty);
    v->ln = conditional_c++;
    v->ty->base = ty;
    return v;
  }

  return new_l_var(tok->str, ty);
}

static Node *func_args(void) {
  if (consume(")"))
    return NULL;

  Node *head = equality();
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
  if (!strcmp(token->str, "bool"))
    return ty_b;
  if (!strcmp(token->str, "void"))
    if (!strcmp(token->next->str, "*"))
      return ty_d_by;
  return NULL;
}

static void consume_ty(Type *ty) {
  // Detector in list of reserved multiple letter string
  if (ty == ty_char || ty == ty_int || ty == ty_b) {
    token = token->next;
  } else if (ty == ty_d_by || ty == ty_vd) {
    token = token->next->next;
  }
}

static Type *expect_ty(void) {
  Type *ty = look_ty();
  if (!ty)
    error_at(token->str, "expected type.");
  consume_ty(ty);
  return ty;
}

static Function *fn(void) {
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
    expect_dec(ty);
    args_c++;
    while (consume(",")) {
      Type *ty = expect_ty();
      expect_dec(ty);
      args_c++;
    }
    expect(')');
  }

  if (startswith(token->str, ";")) {
    expect(';');
    fn->nm = fn_nm;
    return fn;
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
    char *nm = expect_ident();
    new_g_var(nm, ty);
    expect(';');
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
    if (v->is_st) {
      for (Var *m = v->mem; m; m = m->next) {
        i += m->ty->size;
        m->offset = i;
      }
      // struct itself is settled offset same as first member.
      v->offset = v->mem->offset;
    } else {
      // data byte not assigned offset but labeled in section of data.
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
static Node *phase_typ_rev(void) {
  Node *nd = stmt();
  typ_rev(nd);
  return nd;
}

/* stmt = "return" expr ";" */
/*      | "if" "(" cond ")" stmt ("else" stmt)?  */
/*      | "while" "(" cond ")" stmt ";" */
/*      | "struct" "{" (cond)? "}" Ident ";" */
/*      | "{" stmt* "}" */
static Node *stmt(void) {
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

  if (consume("struct")) {
    expect('{');

    Token *t = token;
    // read name of struct in advance.
    while (!consume("}")) {
      token = token->next;
    }

    Var *st_v = expect_init_v(ty_vd);
    token = t;

    // need align for specification of ABI */
    int al_size;
    while (!consume("}")) {
      Type *ty = expect_ty();
      if (al_size < ty->size)
        al_size = ty->size;
      consume_ident();
      expect(';');
    }

    token = t;
    Var *cur = NULL;

    int mem_idx = 0;
    while (!consume("}")) {
      Type *ty = expect_ty();
      Var *v = calloc(1, sizeof(Var));
      v->is_mem = true;
      v->mem_idx = mem_idx++;
      v->ty = ty;
      v->nm = expect_ident();
      v->next = cur;
      cur = v;

      if (v->ty->size < al_size) {
        v->al_size = al_size;
        st_v->ty->size += al_size;
      } else {
        st_v->ty->size += v->ty->size;
      }

      expect(';');
    }
    consume_ident();

    st_v->mem = cur;
    st_v->is_st = true;
    st_v->is_local = true;
    st_v->al_size = al_size;
    expect(';');
    return new_nd(ND_NULL);
  }

  if (consume("enum")) {
    expect('{');
    int e = 1;
    Var *v = new_l_var(expect_ident(), ty_enum);
    v->en = e++;
    while (consume(",")) {
      Var *v = new_l_var(expect_ident(), ty_enum);
      v->en = e++;
    }
    expect('}');
    expect(';');
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
static Node *assign(void) {
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
static Node *add(void) {
  Node *nd = mul();

  if (consume("+")) {
    nd = new_add(nd, add());
  } else if (consume("-")) {
    nd = new_sub(nd, add());
  }

  return nd;
}

/* mul = unary ( "*" unary | "/" unary )* */
static Node *mul(void) {
  Node *nd = postfix();

  if (consume("*")) {
    return new_binary(ND_MUL, nd, mul());
  } else if (consume("/")) {
    return new_binary(ND_DIV, nd, mul());
  }

  return nd;
}

static Node *postfix(void) {
  Node *nd = unary();

  if (consume("++")) {
    return new_uarray(ND_INC, nd);
  } else if (consume("--")) {
    return new_uarray(ND_DEC, nd);
  }

  return nd;
}

/* unary = ( "+" | "-" | "*" | "&" )? primary_expr */
static Node *unary(void) {
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

/* idx          = primary "[" num "]" */
/*              | num "?" cond ":" cond */
static Node *idx(void) {
  Node *nd = primary_expr();

  if (consume("[")) {
    Node *add_nd = new_add(nd, primary_expr());
    Node *nd = new_uarray(ND_REF, add_nd);
    expect(']');
    return nd;
  }

  if (consume("?")) {
    if (nd->kind != ND_NUM)
      error_at(token->str, "\"?:\" operator need number condition before.");
    Node *l = new_num(expect_number());
    expect(':');
    Node *r = new_num(expect_number());
    Node *i = new_nd(ND_IF);
    i->ln = conditional_c++;
    i->cond = nd;
    i->then = l;
    i->els = r;
    return i;
  }

  return nd;
}

/* primary_expr = NUM */
/*              | "(" "{" (stmt)* "}" ")" */
/*              | Ident ("()")? */
/*              | Ident */
/*              | Ident "." */
/*              | ImmString "[" num "]" */
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

    // access member of struct.
    if (consume(".")) {
      Var *st = find_st(tok);
      if (!st)
        error_at(token->str, "can't handle with undefined struct.");
      Var *lv = find_mem(st, consume_ident());
      if (!lv)
        error_at(token->str,
                 "can't handle with undefined variable in member of struct.");

      if (lv->ty == ty_d_by) {
        if (consume("=")) {
          Var *rv = new_str();
          return new_binary(ND_ASSIGN, new_var_nd(lv), new_var_nd(rv));
        }
      }

      Node *l = new_nd(ND_NULL);
      l->v = st;
      Node *r = new_nd(ND_NULL);
      r->v = lv;
      Node *nd = new_binary(ND_MEM, l, r);

      Token *t = token;
      if (!consume("=")) {
        nd->lm = true;
      } else {
        token = t;
      }

      return nd;
    }

    // find variable.
    Var *lv = find_var(tok);
    if (!lv)
      error_at(tok->str, "can't handle with undefined variable.");
    if (lv->ty == ty_enum)
      return new_num(lv->en);
    if (lv->ty == ty_d_by) {
      if (consume("=")) {
        Var *rv = new_str();
        return new_binary(ND_ASSIGN, new_var_nd(lv), new_var_nd(rv));
      }
    }
    return new_var_nd(lv);
  }

  if (consume("sizeof")) {
    Node *nd = calloc(1, sizeof(Node));
    expect('(');

    int b = parse_base(token);
    if (b) {
      token = token->next;
      if (consume("*"))
        ;
      expect(')');
      return new_num(b);
    }

    Token *tok = consume_ident();
    if (!tok)
      error_at(token->str, "expected ident.");
    Var *v = find_var(tok);
    if (v) {
      nd = new_num(v->ty->size);
    } else {
      Var *st = find_st(tok);
      if (st) {
        nd = new_num(st->ty->size);
      } else {
        error_at(token->str, "expected declaration variable.");
      }
    }
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

  if (consume("~")) {
    return new_uarray(ND_TIL, new_num(expect_number()));
  }

  if (consume("!")) {
    return new_uarray(ND_SUR, new_num(expect_number()));
  }

  // case of string literal.
  if (token->kind == TK_STR) {
    // string should parsed as global variable because formed as data-section.
    Var *v = new_str();
    return new_var_nd(v);
  }

  return new_num(expect_number());
}
