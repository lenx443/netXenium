#include "bytecode.h"
#include <stdlib.h>

void bc_free(const Bytecode_Array_ptr bc) {
  if (!bc) return;
  free(bc->bc_array);
  free(bc);
}
