#include <stdlib.h>

#include "bytecode.h"
#include "logs.h"

#define error(msg, ...) log_add(NULL, ERROR, "ByteCode Array", msg, ##__VA_ARGS__)

Bytecode_Array_ptr bc_new() {
  Bytecode_Array_ptr bytecode = malloc(sizeof(Bytecode_Array_t));
  if (!bytecode) {
    error("No hay memoria disponible");
    return NULL;
  }
  bytecode->bc_array = NULL;
  bytecode->bc_size = 0;
  bytecode->bc_capacity = 0;
  return bytecode;
}

void bc_clear(Bytecode_Array_ptr bc) {
  if (!bc) return;
  free(bc->bc_array);
  bc->bc_size = 0;
  bc->bc_capacity = 0;
}

void bc_free(const Bytecode_Array_ptr bc) {
  if (!bc) return;
  free(bc->bc_array);
  free(bc);
}

int bc_add_instr(Bytecode_Array_ptr bc, bc_Instruct_t instr) {
  if (!bc) {
    error("El arreglo de bytecode esta vacío");
    return 0;
  }
  if (bc->bc_size >= bc->bc_capacity) {
    int new_capacity = (bc->bc_capacity == 0) ? 8 : bc->bc_capacity * 2;
    bc_Instruct_ptr new_mem = realloc(bc->bc_array, new_capacity * sizeof(bc_Instruct_t));
    if (!new_mem) {
      error("No se le pudo asignar mas memoria al arreglo de bytecode");
      return 0;
    }
    bc->bc_array = new_mem;
    bc->bc_capacity = new_capacity;
  }
  bc->bc_array[bc->bc_size++] = instr;
  return 1;
}
