#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "block_list.h"
#include "ir_bytecode.h"
#include "ir_instruct.h"
#include "logs.h"
#include "vm_consts.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_string.h"
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

int ir_emit(IR_Bytecode_Array_ptr ir, uint8_t opcode, uint8_t oparg) {
  IR_Instruct_t instr = {opcode, oparg, 0, NULL, 0};
  if (!ir) {
    error("El arreglo de bytecode esta vacío");
    return 0;
  }
  if (ir->ir_size >= ir->ir_capacity) {
    int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
    IR_Instruct_ptr new_mem =
        Xen_Realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
    if (!new_mem) {
      error("No se le pudo asignar mas memoria al arreglo de bytecode");
      return 0;
    }
    ir->ir_array = new_mem;
    ir->ir_capacity = new_capacity;
  }
  ir->ir_array[ir->ir_size++] = instr;
  return 1;
}

int ir_emit_jump(IR_Bytecode_Array_ptr ir, uint8_t opcode,
                 block_node_ptr block) {
  IR_Instruct_t instr = {opcode, 0, 1, block, 0};
  if (!ir) {
    error("El arreglo de bytecode esta vacío");
    return 0;
  }
  if (ir->ir_size >= ir->ir_capacity) {
    int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
    IR_Instruct_ptr new_mem =
        Xen_Realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
    if (!new_mem) {
      error("No se le pudo asignar mas memoria al arreglo de bytecode");
      return 0;
    }
    ir->ir_array = new_mem;
    ir->ir_capacity = new_capacity;
  }
  ir->ir_array[ir->ir_size++] = instr;
  return 1;
}

#ifndef NDEBUG
void ir_print_block(block_node_ptr block, vm_Consts_ptr consts) {
  IR_Bytecode_Array_ptr code = block->instr_array;
  for (Xen_size_t i = 0; i < code->ir_size; i++) {
    printf("%d %ld %s", code->ir_array[i].instr_num, i,
           Instruct_Info_Table[code->ir_array[i].opcode].name);
    if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
        INSTRUCT_FLAG_CO_NAME) {
      if (!consts) {
        printf(" %d (name?)\n", code->ir_array[i].oparg);
      } else {
        Xen_Instance* c_name =
            Xen_Attr_Index_Size_Get(consts->c_names, code->ir_array[i].oparg);
        printf(" %d (%s)\n", code->ir_array[i].oparg,
               c_name ? Xen_String_As_CString(c_name) : "Null");
        if (c_name)
          Xen_DEL_REF(c_name);
      }
    } else if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
               INSTRUCT_FLAG_CO_INSTANCE) {
      if (!consts) {
        printf(" %d (instance?)\n", code->ir_array[i].oparg);
      } else {
        char* val = NULL;
        Xen_Instance* c_inst = Xen_Attr_Index_Size_Get(consts->c_instances,
                                                       code->ir_array[i].oparg);
        if (c_inst) {
          Xen_Instance* string = Xen_Attr_Raw(c_inst);
          if (string) {
            val = Xen_CString_Dup(Xen_String_As_CString(string));
            Xen_DEL_REF(string);
          }
          Xen_DEL_REF(c_inst);
        }
        printf(" %d (%s)\n", code->ir_array[i].oparg, val ? val : "Null");
        if (val)
          Xen_Dealloc(val);
      }
    } else if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
               INSTRUCT_FLAG_ARG) {
      if (code->ir_array[i].is_jump && block->ready) {
        printf(
            " %d\n",
            code->ir_array[i].jump_block->instr_array->ir_array[0].instr_num);
      } else {
        printf(" %d\n", code->ir_array[i].oparg);
      }
    } else {
      printf("\n");
    }
  }
}

void ir_print(block_list_ptr bl) {
  block_node_ptr current = bl->head;
  Xen_size_t b_count = 0;
  while (current) {
    printf("--- Block %lu:\n", b_count++);
    ir_print_block(current, bl->consts);
    current = current->next;
  }
}
#endif
