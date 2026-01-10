#include <stdint.h>
#include <stdio.h>

#include "bytecode.h"
#include "program_code.h"
#include "source_file.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_typedefs.h"

Bytecode_Array_ptr bc_new(void) {
  Bytecode_Array_ptr bytecode = Xen_Alloc(sizeof(Bytecode_Array_t));
  if (!bytecode) {
    return NULL;
  }
  bytecode->bc_array = NULL;
  bytecode->bc_size = 0;
  bytecode->bc_capacity = 0;
  return bytecode;
}

void bc_clear(Bytecode_Array_ptr bc) {
  if (!bc)
    return;
  Xen_Dealloc(bc->bc_array);
  bc->bc_size = 0;
  bc->bc_capacity = 0;
}

void bc_free(const Bytecode_Array_ptr bc) {
  if (!bc)
    return;
  Xen_Dealloc(bc->bc_array);
  Xen_Dealloc(bc);
}

void bc_emit(Bytecode_Array_ptr bc, uint8_t opcode, uint8_t oparg,
             Xen_Source_Address sta) {
  if (!bc) {
    return;
  }
  bc_Instruct_t instr = {{opcode, oparg}, sta};
  if (bc->bc_size >= bc->bc_capacity) {
    int new_capacity = (bc->bc_capacity == 0) ? 8 : bc->bc_capacity * 2;
    bc_Instruct_ptr new_mem =
        Xen_Realloc(bc->bc_array, new_capacity * sizeof(bc_Instruct_t));
    bc->bc_array = new_mem;
    bc->bc_capacity = new_capacity;
  }
  bc->bc_array[bc->bc_size++] = instr;
}

#ifndef NDEBUG
void bc_print(ProgramCode_t pc) {
  Bytecode_Array_ptr code = pc.code;
  for (Xen_size_t i = 0; i < code->bc_size; i++) {
    if (code->bc_array[i].hdr.bci_opcode <= HALT) {
      printf("%ld %s %u\n", i,
             Instruct_Info_Table[code->bc_array[i].hdr.bci_opcode].name,
             code->bc_array[i].hdr.bci_oparg);
    } else {
      printf("%ld %02X %u\n", i, code->bc_array[i].hdr.bci_opcode,
             code->bc_array[i].hdr.bci_oparg);
    }
  }
}
#endif
