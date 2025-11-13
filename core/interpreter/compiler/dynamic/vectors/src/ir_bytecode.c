#include <stdint.h>

#include "ir_bytecode.h"
#include "ir_instruct.h"
#include "logs.h"
#include "xen_alloc.h"
#include "xen_typedefs.h"

#define error(msg, ...) log_add(NULL, ERROR, "IR Array", msg, ##__VA_ARGS__)

IR_Bytecode_Array_ptr ir_new() {
  IR_Bytecode_Array_ptr intr_array = Xen_Alloc(sizeof(IR_Bytecode_Array_t));
  if (!intr_array) {
    error("No hay memoria disponible");
    return NULL;
  }
  intr_array->ir_array = NULL;
  intr_array->ir_size = 0;
  intr_array->ir_capacity = 0;
  return intr_array;
}

void ir_free(const IR_Bytecode_Array_ptr ir) {
  if (!ir)
    return;
  Xen_Dealloc(ir->ir_array);
  Xen_Dealloc(ir);
}

Xen_ssize_t ir_emit(IR_Bytecode_Array_ptr ir, Xen_ssize_t offset,
                    uint8_t opcode, uint8_t oparg) {
  IR_Instruct_t instr = {opcode, oparg, 0, NULL, 0};
  if (!ir) {
    error("El arreglo de bytecode esta vacío");
    return -1;
  }
  if (offset == -1) {
    if (ir->ir_size >= ir->ir_capacity) {
      int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
      IR_Instruct_ptr new_mem =
          Xen_Realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
      if (!new_mem) {
        error("No se le pudo asignar mas memoria al arreglo de bytecode");
        return -1;
      }
      ir->ir_array = new_mem;
      ir->ir_capacity = new_capacity;
    }
    Xen_size_t result = ir->ir_size;
    ir->ir_array[ir->ir_size++] = instr;
    return result;
  } else {
    if (offset < 0 || (Xen_size_t)offset >= ir->ir_size) {
      return -1;
    }
    ir->ir_array[offset] = instr;
    return offset;
  }
}
