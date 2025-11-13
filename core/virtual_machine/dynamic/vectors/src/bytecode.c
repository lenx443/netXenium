#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "attrs.h"
#include "bytecode.h"
#include "instance.h"
#include "logs.h"
#include "program_code.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_string.h"
#include "xen_typedefs.h"

#define error(msg, ...)                                                        \
  log_add(NULL, ERROR, "ByteCode Array", msg, ##__VA_ARGS__)

Bytecode_Array_ptr bc_new() {
  Bytecode_Array_ptr bytecode = Xen_Alloc(sizeof(Bytecode_Array_t));
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

int bc_emit(Bytecode_Array_ptr bc, uint8_t opcode, uint8_t oparg) {
  if (!bc) {
    error("El arreglo de bytecode esta vacío");
    return 0;
  }
  bc_Instruct_t instr = {{opcode, oparg}};
  if (bc->bc_size >= bc->bc_capacity) {
    int new_capacity = (bc->bc_capacity == 0) ? 8 : bc->bc_capacity * 2;
    bc_Instruct_ptr new_mem =
        Xen_Realloc(bc->bc_array, new_capacity * sizeof(bc_Instruct_t));
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

#ifndef NDEBUG
void bc_print(ProgramCode_t pc) {
  Bytecode_Array_ptr code = pc.code;
  for (Xen_size_t i = 0; i < code->bc_size; i++) {
    printf("%ld %s", i, Instruct_Info_Table[code->bc_array[i].bci_opcode].name);
    if (Instruct_Info_Table[code->bc_array[i].bci_opcode].flags &
        INSTRUCT_FLAG_CO_NAME) {
      Xen_Instance* c_name = Xen_Attr_Index_Size_Get(
          pc.consts->c_names, code->bc_array[i].bci_oparg);
      printf(" %d (%s)\n", code->bc_array[i].bci_oparg,
             c_name ? Xen_String_As_CString(c_name) : "Null");
      if (c_name)
        Xen_DEL_REF(c_name);
    } else if (Instruct_Info_Table[code->bc_array[i].bci_opcode].flags &
               INSTRUCT_FLAG_CO_INSTANCE) {
      char* val = NULL;
      Xen_Instance* c_inst = Xen_Attr_Index_Size_Get(
          pc.consts->c_instances, code->bc_array[i].bci_oparg);
      if (c_inst) {
        Xen_Instance* string = Xen_Attr_Raw(c_inst);
        if (string) {
          val = strdup(Xen_String_As_CString(string));
          Xen_DEL_REF(string);
        }
        Xen_DEL_REF(c_inst);
      }
      printf(" %d (%s)\n", code->bc_array[i].bci_oparg, val ? val : "Null");
      if (val)
        Xen_Dealloc(val);
    } else if (Instruct_Info_Table[code->bc_array[i].bci_opcode].flags &
               INSTRUCT_FLAG_ARG) {
      printf(" %d\n", code->bc_array[i].bci_oparg);
    } else {
      printf("\n");
    }
  }
}
#endif
