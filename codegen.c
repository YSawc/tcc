#include "tcc.h"

static Node *expr(void);
static Node *mul(void);

static Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_num(long val, Token *tok) {
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

/* expr = mul ("+" mul | "-" mul)* */
static Node *expr() {
  Node *node = mul();
  if (consume("+")) {
    return new_binary(ND_SUB, node, mul(), token);
  } else if (consume("-")) {
    return new_binary(ND_SUB, new_num(0, token), mul(), token);
  }
  return node;
}

/* mul  = num ("*" num | "/" num)* */
static Node *mul(void) {
  if (token->kind == TK_NUM) {
    return new_num(expect_number(), token);
  }
  return NULL;
}

Node *node_gen() {
  Node *node = expr();
  return node;
}

void code_gen(Node *node) {
    switch (node->kind) {
    case ND_ADD:
      printf("  add rax, ");
    case ND_SUB:
      printf("  sub rax, ");
    case ND_NUM:
      printf("  mov rax, %ld\n", node->val);
    }
}
