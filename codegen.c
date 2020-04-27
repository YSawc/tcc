#include "tcc.h"

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

static Node *new_num(long val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *stmt(void);
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

/* relational stmt */
static Node *stmt() {
  Node *node = equality();
  expect(';');
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

/* primary_expr = ( NUM | "(" add ")" ) */
static Node *primary_expr(void) {
  if (token->kind == TK_NUM) {
    return new_num(expect_number());
  } else if (consume("(")) {
    node = add();
    expect(')');
    return node;
  } else if (token->kind == TK_RPAREN) {
    return new_node(ND_RPAREN);
  }
  return NULL;
}

void code_gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %ld\n", node->val);
    return;
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
