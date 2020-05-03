#include "tcc.h"

Var *lVars;

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static char *func_name;

// Pushes the given node's address to the stack.
static void gen_var_addr(Node *node) {
  printf("  lea rax, [rbp-%d]\n", node->var->offset);
  printf("  push rax\n");
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

Var *new_lvar(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = lVars;
  var->type = *type_int;
  var->name = name;
  lVars = var;
  return var;
}

static Var *find_var(Token *tok) {
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

static Node *new_stmt_node(NodeKind kind, Node *cond, Node *stmt) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->cond = cond;
  node->stmt = stmt;
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
  node->str = var->name;
  return node;
}

static Node *expect_dec() {
  Token *tok = consume_ident();
  Var *var = find_var(tok);
  if (var)
    error_at(tok->str, "multiple declaration.");
  var = new_lvar(tok->str);
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

// program = stmt*
Function *gen_program(void) {
  lVars = NULL;

  Node head = {};
  Node *cur = &head;

  expect_str("int");
  func_name = expect_ident();
  expect('(');
  expect(')');
  expect('{');

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  Function *prog = calloc(1, sizeof(Function));
  prog->name = func_name;
  prog->node = head.next;
  prog->lVars = lVars;
  return prog;
}

void assign_var_offset(Function *function) {
  int i = 0;
  for (Var *v = function->lVars; v; v = v->next) {
    i++;
    v->offset = i * 8;
  }
}

// emit rsp counts of variables in function.
void emit_rsp(Function *function) {
  // shift rsp counter counts of local variable.
  if (function->lVars) {
    int i = 0;
    for (Var *v = function->lVars; v; v = v->next)
      i++;
    printf("  sub rsp, %d\n", i * 8);
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

/* add = mul ( "*" mul | "/" mul )* */
static Node *add() {
  Node *node = mul();

  if (consume("+")) {
    node = new_binary(ND_ADD, node, primary_expr());
  } else if (consume("-")) {
    node = new_binary(ND_SUB, node, primary_expr());
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
    Token *tok = consume_ident();
    if (!tok)
      error_at(tok->str, "ident not found.");
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_ADDR;
    node->var = find_var(tok);
    return node;
  } else {
    return primary_expr();
  }
}

/* primary_expr = NUM */
/*              | Ident ("()")? */
/*              | Ident */
/*              | "(" add ")" ) */
/*              | "sizeof" "(" add ")" */
static Node *primary_expr(void) {
  if (consume("(")) {
    node = add();
    expect(')');
    return node;
  }

  if (consume("int"))
    return expect_dec();

  Token *tok = consume_ident();

  if (tok) {
    // for function.
    if (consume("(")) {
      Node *node = new_node(ND_FNC);
      node->str = tok->str;
      node->args = func_args();
      return node;
    }

    // find variable. If detected, return var_node.
    Var *var = find_var(tok);
    if (!var)
      var = new_lvar(tok->str);
    return new_var_node(var);
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
    node = new_num(var->type.type_size);
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
    gen_var_addr(node);
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

    printf("  call %s\n", node->str);
    printf("  push rax\n");
    return;
  }
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
  case ND_SUB:
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
