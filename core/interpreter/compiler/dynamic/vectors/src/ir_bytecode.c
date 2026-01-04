#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "block_list.h"
#include "ir_bytecode.h"
#include "ir_instruct.h"
#include "source_file.h"
#include "vm_consts.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

IR_Bytecode_Array_ptr ir_new(void) {
  IR_Bytecode_Array_ptr intr_array = Xen_Alloc(sizeof(IR_Bytecode_Array_t));
  if (!intr_array) {
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

int ir_emit(IR_Bytecode_Array_ptr ir, Xen_uint8_t opcode, Xen_ulong_t oparg,
            Xen_Source_Address sta) {
  IR_Instruct_t instr = {opcode, oparg, 0, NULL, 0, sta};
  if (!ir) {
    return 0;
  }
  if (ir->ir_size >= ir->ir_capacity) {
    int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
    IR_Instruct_ptr new_mem =
        Xen_Realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
    if (!new_mem) {
      return 0;
    }
    ir->ir_array = new_mem;
    ir->ir_capacity = new_capacity;
  }
  ir->ir_array[ir->ir_size++] = instr;
  return 1;
}

int ir_emit_jump(IR_Bytecode_Array_ptr ir, uint8_t opcode, block_node_ptr block,
                 Xen_Source_Address sta) {
  IR_Instruct_t instr = {opcode, 0, 1, block, 0, sta};
  if (!ir) {
    return 0;
  }
  if (ir->ir_size >= ir->ir_capacity) {
    int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
    IR_Instruct_ptr new_mem =
        Xen_Realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
    if (!new_mem) {
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
    printf("%ld %ld %s", code->ir_array[i].instr_num, i,
           Instruct_Info_Table[code->ir_array[i].opcode].name);
    if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
        INSTRUCT_FLAG_CO_NAME) {
      if (!consts) {
        printf(" %ld (name?)\n", code->ir_array[i].oparg);
      } else {
        Xen_Instance* c_name = Xen_Vector_Get_Index(
            (Xen_Instance*)consts->c_names->ptr, code->ir_array[i].oparg);
        printf(" %ld (%s)\n", code->ir_array[i].oparg,
               c_name ? Xen_String_As_CString(c_name) : "Null");
      }
    } else if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
               INSTRUCT_FLAG_CO_INSTANCE) {
      if (!consts) {
        printf(" %ld (instance?)\n", code->ir_array[i].oparg);
      } else {
        char* val = NULL;
        Xen_Instance* c_inst = Xen_Vector_Get_Index(
            (Xen_Instance*)consts->c_instances->ptr, code->ir_array[i].oparg);
        if (c_inst) {
          Xen_Instance* string = Xen_Attr_Raw(c_inst);
          if (string) {
            val = Xen_CString_Dup(Xen_String_As_CString(string));
          }
        }
        printf(" %ld (%s)\n", code->ir_array[i].oparg, val ? val : "Null");
        if (val)
          Xen_Dealloc(val);
      }
    } else if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
               INSTRUCT_FLAG_CO_CALLABLE) {
      printf(" %ld (Callable)\n", code->ir_array[i].oparg);
    } else if (Instruct_Info_Table[code->ir_array[i].opcode].flags &
               INSTRUCT_FLAG_ARG) {
      if (code->ir_array[i].is_jump && block->ready) {
        printf(
            " %ld\n",
            code->ir_array[i].jump_block->instr_array->ir_array[0].instr_num);
      } else {
        printf(" %ld\n", code->ir_array[i].oparg);
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
