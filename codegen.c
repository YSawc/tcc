#include "tcc.h"

static Var *lVars;
static Var *gVars;

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static char *func_name;

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *node) {
  Var *var = node->var;
  if (node->kind == ND_REF) {
    code_gen(node->rhs);
  } else if (var->is_local) {
    printf("  lea rax, [rbp-%d]\n", var->offset);
    printf("  push rax\n");
  } else {
    printf("  push offset %s\n", var->name);
  }
  return;
}

static void load_64(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

static void load_8(void) {
  printf("  pop rax\n");
  printf("  movsx rax, byte ptr [rax]\n");
  printf("  push rax\n");
}

static void store_val(Type *typ) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (typ->kind == TYP_CHAR_ARR)
    printf("  mov [rax], dil\n");
  else
    printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

static Var *new_lvar(char *name, Type *typ) {
  if (!typ)
    error_at(token->str, "expected type, but not detected.");

  Var *var = calloc(1, sizeof(Var));
  var->next = lVars;
  var->typ = typ;

  var->name = name;
  var->is_local = true;
  lVars = var;
  return var;
}
static Var *new_arr_var(char *name, int l, Type *typ) {
  Var *ret_v;
  Var *var = calloc(1, sizeof(Var));
  var->next = lVars;
  if (typ == typ_char)
    var->typ = typ_char_arr;
  else if (typ == typ_int)
    var->typ = typ_int_arr;
  var->name = name;
  var->is_local = true;
  lVars = var;

  for (int l_i = 0; l_i < l; l_i++) {
    Var *tmp = calloc(1, sizeof(Var));
    tmp->next = lVars;
    tmp->typ = var->typ;
    tmp->name = "";
    tmp->is_local = true;
    lVars = tmp;

    if (l_i == l - 1)
      ret_v = tmp;
  }
  return ret_v;
}

// find variable scope of a whole
static Var *find_var(Token *tok) {
  for (Var *v = lVars; v; v = v->next)
    if (strlen(v->name) == tok->len && !strncmp(v->name, tok->str, tok->len))
      return v;

  for (Var *v = gVars; v; v = v->next)
    if (strlen(v->name) == tok->len && !strncmp(v->name, tok->str, tok->len))
      return v;
  return NULL;
}

// find variable scope of local
static Var *find_lvar(Token *tok) {
  for (Var *v = lVars; v; v = v->next)
    if (strlen(v->name) == tok->len && !strncmp(v->name, tok->str, tok->len))
      return v;
  return NULL;
}

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_uarray(NodeKind kind, Node *rhs) {
  Node *node = new_node(kind);
  node->rhs = rhs;
  return node;
}

static Node *new_num(long val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_var_node(Var *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  node->typ = var->typ;
  node->str = var->name;
  if (var->typ->base)
    node->typ->base = var->typ->base;
  return node;
}

static Node *expect_dec(Type *typ) {
  Token *tok = consume_ident();
  Var *var = find_lvar(tok);
  if (var)
    error_at(tok->str, "multiple declaration.");

  if (consume("[")) {
    int l = expect_number();
    expect(']');
    var = new_arr_var(tok->str, l, typ);
    var->typ->base = typ;
    Node *node = new_node(ND_NULL);
    node->var = var;
    return node;
  }
  var = new_lvar(tok->str, typ);
  // In this case, it only declared but not assigned.
  if (startswith(token->str, ";"))
    return new_node(ND_NULL);
  return new_var_node(var);
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

static bool is_function(void) {
  Token *tok = token;
  expect_str("int");
  bool isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
}

static Type *look_type(void) {
  if (strcmp(token->str, "int") == 0)
    return typ_int;
  if (strcmp(token->str, "char") == 0)
    return typ_char;
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

static void global_var() {
  expect_str("int");
  char *name = expect_ident();
  expect(';');
  Var *var = calloc(1, sizeof(Var));
  var->next = gVars;
  var->typ = typ_int;
  var->name = name;
  gVars = var;
}

static Function *function() {
  lVars = NULL;

  Function *func = calloc(1, sizeof(Function));
  Node head_node = {};
  Node *cur_node = &head_node;

  // in this scope, be in internal of function.
  expect_str("int");
  func_name = expect_ident();
  expect('(');
  expect(')');
  expect('{');
  while (!consume("}")) {
    cur_node->next = phase_typ_rev();
    cur_node = cur_node->next;
  }

  func->name = func_name;
  func->node = head_node.next;
  func->lVars = lVars;
  return func;
}

// program = stmt*
Program *gen_program(void) {
  gVars = NULL;

  Function head_func = {};
  Function *func_cur = &head_func;

  // detect global functoin.
  while (!is_function()) {
    global_var();
  }

  while (!at_eof()) {
    func_cur->next = function();
    func_cur = func_cur->next;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->func = head_func.next;
  prog->gVars = gVars;
  return prog;
}

void assign_var_offset(Function *function) {
  int i = 0;
  for (Var *v = function->lVars; v; v = v->next) {
    i += v->typ->size;
    v->offset = i;
  }
}

// emit rsp counts of variables in function.
void emit_rsp(Function *function) {
  // shift rsp counter counts of local variable.
  if (function->lVars) {
    int i = 0;
    // rsp is multiples of 8. So rsp need roud up with 8 as nardinality.
    for (Var *v = function->lVars; v; v = v->next)
      i += v->typ->size;
    i = ((i + 8 - 1) / 8) * 8;
    printf("  sub rsp, %d\n", i);
  } else {
    printf("  sub rsp, 0\n");
  };
}

// In this phase, reveal type of each returned node.
static Node *phase_typ_rev() {
  Node *node = stmt();
  typ_rev(node);
  return node;
}

/* stmt = "return" expr ";" */
/*      | "if" "(" cond ")" stmt ("else" stmt)?  */
/*      | "while" "(" cond ")" stmt ";" */
/*      | "{" stmt* "}" */
static Node *stmt() {
  if (consume("return")) {
    Node *node = new_uarray(ND_RETURN, assign());
    expect(';');
    return node;
  }

  if (consume("if")) {
    Node *node = new_node(ND_IF);
    expect('(');
    node->cond = relational();
    expect(')');
    node->then = stmt();
    if (consume("else")) {
      node->els = stmt();
    }
    return node;
  }

  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    expect('(');
    node->cond = equality();
    expect(')');
    node->stmt = stmt();
    return node;
  }

  if (consume("{")) {
    Node head = {};
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK);
    node->block = head.next;
    return node;
  }

  Node *node = assign();
  expect(';');
  return node;
}

// assign = equality ("=" assign)?
static Node *assign() {
  Node *node = equality();

  if (consume("=")) {
    Node *n = new_binary(ND_ASSIGN, node, add());
    return n;
  }
  return node;
}

/* equality = relational ("==" relational | "!=" relational )* */
static Node *equality(void) {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_binary(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_binary(ND_NEQ, node, relational());
    } else {
      return node;
    }
  }
};

/* relational = add ("<" add | "<=" add | ">" add | ">=" add)* */
static Node *relational(void) {
  Node *node = add();
  for (;;) {
    if (consume("<")) {
      node = new_binary(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_binary(ND_LTE, node, add());
    } else if (consume(">")) {
      node = new_binary(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_binary(ND_LTE, add(), node);
    } else {
      return node;
    }
  }
};

static Node *new_add(Node *lhs, Node *rhs) {
  typ_rev(lhs);
  typ_rev(rhs);

  if (is_integer(lhs->typ) && is_integer(rhs->typ))
    return new_binary(ND_ADD, lhs, rhs);
  if (lhs->typ->base && is_integer(rhs->typ))
    return new_binary(ND_PTR_ADD, lhs, rhs);
  if (is_integer(lhs->typ) && rhs->typ->base)
    return new_binary(ND_PTR_ADD, rhs, lhs);
  error_at(token->str, "invalid operands");
  return NULL;
}

static Node *new_sub(Node *lhs, Node *rhs) {
  typ_rev(lhs);
  typ_rev(rhs);

  if (is_integer(lhs->typ) && is_integer(rhs->typ))
    return new_binary(ND_SUB, lhs, rhs);
  if (lhs->typ->base && is_integer(rhs->typ))
    return new_binary(ND_PTR_SUB, lhs, rhs);
  if (is_integer(lhs->typ) && rhs->typ->base)
    return new_binary(ND_PTR_SUB, rhs, lhs);
  error_at(token->str, "invalid operands");
  return NULL;
}

/* add = mul ( "*" mul | "/" mul )* */
static Node *add() {
  Node *node = mul();

  if (consume("+")) {
    node = new_add(node, primary_expr());
  } else if (consume("-")) {
    node = new_sub(node, primary_expr());
  }

  return node;
}

/* mul = unary ( "*" unary | "/" unary )* */
static Node *mul() {
  Node *node = unary();

  if (consume("*")) {
    node = new_binary(ND_MUL, node, primary_expr());
  } else if (consume("/")) {
    node = new_binary(ND_DIV, node, primary_expr());
  }

  return node;
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
    return node;
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
static Node *primary_expr(void) {
  if (consume("(")) {
    if (consume("{")) {
      // read gnu statement.
      Node head = {};
      Node *cur = &head;

      while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
      }

      expect(')');
      Node *node = new_node(ND_GNU_BLOCK);
      node->block = head.next;
      return node;
    } else {
      node = add();
      expect(')');
      return node;
    }
  }

  Type *typ = look_type();
  if (typ) {
    consume_typ();
    return expect_dec(typ);
  }

  Token *tok = consume_ident();

  if (tok) {
    // find function.
    if (consume("(")) {
      Node *node = new_node(ND_FNC);
      node->str = tok->str;
      node->args = func_args();
      return node;
    }

    // find variable.
    Var *var = find_var(tok);
    if (var)
      return new_var_node(var);
    error_at(token->str, "can't handle with undefined variable.");
  }

  if (consume("sizeof")) {
    Node *node = calloc(1, sizeof(Node));
    expect('(');
    Token *tok = consume_ident();
    if (!tok)
      error_at(tok->str, "expected ident.");
    Var *var = find_var(tok);
    if (!var)
      error_at(tok->str, "expected declaration variable.");
    node = new_num(var->typ->size);
    expect(')');
    return node;
  }

  return new_num(expect_number());
}

static int conditional_c = 0;

void code_gen(Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_VAR:
    gen_var_addr(node);
    if (node->typ->kind != TYP_INT_ARR && node->typ->kind != TYP_CHAR_ARR)
      load_64();
    return;
  case ND_ASSIGN:
    gen_var_addr(node->lhs);
    code_gen(node->rhs);
    store_val(node->lhs->typ);
    if (node->lhs->typ->kind == TYP_INT_ARR ||
        node->lhs->typ->kind == TYP_CHAR_ARR)
      printf("  add rsp, 8\n");
    return;
  case ND_ADDR:
    gen_var_addr(node->rhs);
    return;
  case ND_REF:
    code_gen(node->rhs);
    if (node->typ->kind == TYP_CHAR_ARR)
      load_8();
    else
      load_64();
    return;
  case ND_IF:
    conditional_c++;
    if (node->els) {
      code_gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.else.%d\n", conditional_c);
      code_gen(node->then);
      printf("  jmp .L.end.%d\n", conditional_c);
      printf(".L.else.%d:\n", conditional_c);
      code_gen(node->els);
      printf(".L.end.%d:\n", conditional_c);
    } else {
      code_gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.end.%d\n", conditional_c);
      code_gen(node->then);
      printf(".L.end.%d:\n", conditional_c);
    }
    return;
  case ND_WHILE:
    conditional_c++;
    printf(".L.begin.%d:\n", conditional_c);
    code_gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.end.%d\n", conditional_c);
    code_gen(node->stmt);
    printf("  jmp .L.begin.%d\n", conditional_c);
    printf(".L.end.%d:\n", conditional_c);
    return;
  case ND_FNC: {
    int args_c = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      code_gen(arg);
      args_c++;
    }

    for (int i = args_c - 1; i >= 0; i--)
      printf("  pop %s\n", arg_regs[i]);
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.1\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->str);
    printf("  jmp .L.end.1\n");
    printf(".L.call.1:\n");
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->str);
    printf("  add rsp, 8\n");
    printf(".L.end.1:\n");
    printf("  push rax\n");
    printf("  push rax\n");
    return;
  }
  case ND_GNU_BLOCK:
    for (Node *n = node->block; n; n = n->next)
      code_gen(n);
    return;
  case ND_BLOCK:
    for (Node *n = node->block; n; n = n->next)
      code_gen(n);
    return;
  case ND_RETURN:
    code_gen(node->rhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", func_name);
    return;
  default:;
  }

  code_gen(node->lhs);
  code_gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_PTR_ADD:
    if (lr_if_or(node, TYP_CHAR_ARR))
      printf("  imul rdi, 1\n");
    else
      printf("  imul rdi, 8\n");
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
    if (lr_if_or(node, TYP_INT_ARR))
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
    error_at(node->str, "can't generate code from this node.\n");
  }

  printf("  push rax\n");
}
