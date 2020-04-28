#include "tcc.h"

// Pushes the given node's address to the stack.
static void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    int offset = (node->str[0] - 'a' + 1) * 8;
    printf("  lea rax, [rbp-%d]\n", offset);
    printf("  push rax\n");
    return;
  }

  error_at(token->str, "not an lvalue");
}

static void load(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

static void store(void) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
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

static Node *new_var_node(char *str) {
  Node *node = new_node(ND_VAR);
  node->str = str;
  return node;
}

static Node *stmt(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary_expr(void);

Node *node_gen() {
  Node *node = stmt();
  return node;
}

/* stmt ("return")* relational ";" */
static Node *stmt() {
  if (consume("return")) {
    Node *node = new_uarray(ND_RETURN, equality());
    expect(';');
    return node;
  }

  Node *node = assign();
  expect(';');
  return node;
}

// assign = equality ("=" assign)?
static Node *assign(void) {
  Node *node = equality();
  if (consume("="))
    node = new_binary(ND_ASSIGN, node, assign());
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

  for (;;) {
    if (consume("+")) {
      node = new_binary(ND_ADD, node, primary_expr());
    } else if (consume("-")) {
      node = new_binary(ND_SUB, node, primary_expr());
    } else {
      return node;
    }
  }
}

/* mul = unary ( "*" unary | "/" unary )* */
static Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_binary(ND_MUL, node, primary_expr());
    } else if (consume("/")) {
      node = new_binary(ND_DIV, node, primary_expr());
    } else {
      return node;
    }
  }
}

/* unary = ( "+" | "-" )? primary_expr */
static Node *unary() {
  for (;;) {
    if (consume("+")) {
      return unary();
    } else if (consume("-")) {
      return new_binary(ND_SUB, new_num(0), unary());
    } else {
      return primary_expr();
    }
  }
}

/* primary_expr = ( NUM | Ident | "(" add ")" ) */
static Node *primary_expr(void) {
  if (consume("(")) {
    node = add();
    expect(')');
    return node;
  }

  Token *tok = consume_ident();
  if (tok)
    return new_var_node(tok->str);

  return new_num(expect_number());
}

void code_gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_VAR:
    gen_addr(node);
    load();
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    code_gen(node->rhs);
    store();
    return;
  case ND_RETURN:
    code_gen(node->rhs);
    printf("  pop rax\n");
    printf("  jmp .L.return\n");
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
