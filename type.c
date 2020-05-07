#include "tcc.h"

Type *typ_char = &(Type){TYP_CHAR, 1};
Type *typ_int = &(Type){TYP_INT, 8};

bool is_integer(Type *typ) { return typ->kind == TYP_INT; }

Type *pointer_to(Type *base) {
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TYP_PTR;
  typ->base = base;
  return typ;
}

void typ_rev(Node *node) {
  if (!node || node->typ)
    return;

  typ_rev(node->lhs);
  typ_rev(node->rhs);
  typ_rev(node->cond);
  typ_rev(node->stmt);

  typ_rev(node->args);

  typ_rev(node->els);
  typ_rev(node->then);

  for (Node *n = node->args; n; n = n->next)
    typ_rev(n);
  for (Node *n = node->block; n; n = n->next)
    typ_rev(n);

  switch (node->kind) {
  case ND_NUM:
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LTE:
  case ND_VAR:
  case ND_FNC:
    node->typ = typ_int;
    return;
  case ND_PTR_ADD:
  case ND_PTR_SUB:
  case ND_ASSIGN:
    node->typ = node->lhs->typ;
    return;
  case ND_ADDR:
    node->typ = pointer_to(node->rhs->typ);
    return;
  case ND_REF:
    if (node->rhs->typ->kind == TYP_PTR)
      node->typ = node->rhs->typ->base;
    else
      node->typ = typ_int;
    return;
  default:
    return;
  }
}
