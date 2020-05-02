#include "tcc.h"

int type_size(Var *var) {
  if (var->typeKind == TYPE_INT)
    return 8;
  return 0;
};
