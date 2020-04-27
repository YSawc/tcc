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

static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary_expr(void);

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

Node *node_gen() {
  Node *node = add();
  return node;
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
  default:
    error_at(node->str, "can't generate code from this node.\n");
  }

  printf("  push rax\n");
}
