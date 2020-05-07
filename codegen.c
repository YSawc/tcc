#include "tcc.h"

Var *lVars;
Var *gVars;

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static char *func_name;

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *node) {
  Var *var = node->var;
  if (var->is_local) {
    printf("  lea rax, [rbp-%d]\n", var->offset);
    printf("  push rax\n");
  } else {
    printf("  push offset %s\n", var->name);
  }
  return;
}

static void load_val(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

static void store_val(void) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

static Var *new_lvar(char *name, char *type_s) {
  Var *var = calloc(1, sizeof(Var));
  var->next = lVars;

  if (startswith(type_s, "int")) {
    var->type = typ_int;
  } else if (startswith(type_s, "char")) {
    var->type = typ_char;
  } else {
    error_at(token->str, "expected type, but not detected.");
  }

  var->name = name;
  var->is_local = true;
  lVars = var;
  return var;
}

static Var *find_lvar(Token *tok) {
  for (Var *v = lVars; v; v = v->next)
    if (strlen(v->name) == tok->len && !strncmp(v->name, tok->str, tok->len))
      return v;
  return NULL;
}

static Var *find_gvar(Token *tok) {
  for (Var *v = gVars; v; v = v->next)
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
  node->typ = var->type;
  node->str = var->name;
  return node;
}

static Node *expect_dec(char *type_s) {
  Token *tok = consume_ident();
  Var *var = find_lvar(tok);
  if (var)
    error_at(tok->str, "multiple declaration.");
  var = new_lvar(tok->str, type_s);
  return new_var_node(var);
}

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
    /* cur->next = new_num(expect_number()); */
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

static char *look_type(void) {
  // list of reserved multiple letter.string
  static char *types[] = {"int", "char"};
  char *s = NULL;

  // Detector in list of reserved multiple letter string
  for (int i = 0; i < sizeof(types) / sizeof(*types); i++) {
    if (startswith(token->str, types[i])) {
      s = types[i];
      break;
    }
  }
  return s;
}

static void global_var() {
  expect_str("int");
  char *name = expect_ident();
  expect(';');
  Var *var = calloc(1, sizeof(Var));
  var->next = gVars;
  var->type = typ_int;
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
    cur_node->next = stmt();
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
    i += v->type->size;
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
      i += v->type->size;
    i = ((i + 8 - 1) / 8) * 8;
    printf("  sub rsp, %d\n", i);
  } else {
    printf("  sub rsp, 0\n");
  };
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
  /* } */
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
    /* Token *tok = consume_ident(); */
    /* if (!tok) */
    /* error_at(tok->str, "ident not found."); */
    /* Node *node = calloc(1, sizeof(Node)); */
    /* node->kind = ND_ADDR; */
    /* node->var = find_lvar(tok); */
    /* node->rhs->typ = node->var->type; */
    return node;
  } else {
    return primary_expr();
  }
}

/* primary_expr = NUM */
/*              | "(" "{" (stmt)* "}" ")" */
/*              | Ident ("()")? */
/*              | Ident */
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

  char *type_s = look_type();
  if (type_s) {
    consume(type_s);
    return expect_dec(type_s);
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

    // find local variable.
    Var *lVar = find_lvar(tok);
    if (lVar)
      return new_var_node(lVar);

    // find gloval variable.
    Var *gVar = find_gvar(tok);
    if (gVar)
      return new_var_node(gVar);
    error_at(token->str, "can't handle with undefined variable.");
  }

  if (consume("sizeof")) {
    Node *node = calloc(1, sizeof(Node));
    expect('(');
    Token *tok = consume_ident();
    if (!tok)
      error_at(tok->str, "expected ident.");
    Var *var = find_lvar(tok);
    if (!var)
      error_at(tok->str, "expected declaration variable.");
    node = new_num(var->type->size);
    expect(')');
    return node;
  }

  return new_num(expect_number());
}

static int conditional_c = 0;

void code_gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_VAR:
    gen_var_addr(node);
    load_val();
    return;
  case ND_ASSIGN:
    gen_var_addr(node->lhs);
    code_gen(node->rhs);
    store_val();
    return;
  case ND_ADDR:
    gen_var_addr(node->rhs);
    return;
  case ND_REF:
    code_gen(node->rhs);
    load_val();
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
    printf("  imul rdi, 8\n");
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
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
