#include <stdio.h>
#include <stdlib.h>

#include "bytecode.h"
#include "implement.h"
#include "instance.h"
#include "logs.h"
#include "operators.h"
#include "program_code.h"
#include "vm.h"
#include "vm_instructs.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_typedefs.h"

#define error(msg, ...)                                                        \
  log_add(NULL, ERROR, "ByteCode Array", msg, ##__VA_ARGS__)

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
  if (!bc)
    return;
  free(bc->bc_array);
  bc->bc_size = 0;
  bc->bc_capacity = 0;
}

void bc_free(const Bytecode_Array_ptr bc) {
  if (!bc)
    return;
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
    bc_Instruct_ptr new_mem =
        realloc(bc->bc_array, new_capacity * sizeof(bc_Instruct_t));
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
      Xen_Instance* c_name = Xen_Operator_Eval_Pair_Steal2(
          pc.consts->c_names, Xen_Number_From_UInt(code->bc_array[i].bci_oparg),
          Xen_OPR_GET_INDEX);
      printf(" %d (%s)\n", code->bc_array[i].bci_oparg,
             c_name ? Xen_String_As_CString(c_name) : "Null");
      if (c_name)
        Xen_DEL_REF(c_name);
    } else if (Instruct_Info_Table[code->bc_array[i].bci_opcode].flags &
               INSTRUCT_FLAG_CO_INSTANCE) {
      char* val = NULL;
      Xen_Instance* c_inst = Xen_Operator_Eval_Pair_Steal2(
          pc.consts->c_instances,
          Xen_Number_From_UInt(code->bc_array[i].bci_oparg), Xen_OPR_GET_INDEX);
      if (c_inst) {
        Xen_Instance* string =
            vm_call_native_function(Xen_TYPE(c_inst)->__raw, c_inst, nil);
        if (string) {
          val = strdup(Xen_String_As_CString(string));
          Xen_DEL_REF(string);
        }
        Xen_DEL_REF(c_inst);
      }
      printf(" %d (%s)\n", code->bc_array[i].bci_oparg, val ? val : "Null");
      if (val)
        free(val);
    } else if (Instruct_Info_Table[code->bc_array[i].bci_opcode].flags &
               INSTRUCT_FLAG_ARG) {
      printf(" %d\n", code->bc_array[i].bci_oparg);
    } else {
      printf("\n");
    }
  }
}
#endif
