#ifndef __IR_BYTECODE_H__
#define __IR_BYTECODE_H__

#include <stddef.h>

#include "ir_instruct.h"
#include "source_file.h"
#include "vm_consts.h"
#include "xen_typedefs.h"

struct block_node;
struct block_list;

struct IR_Bytecode_Array {
  struct IR_Instruct* ir_array;
  size_t ir_size;
  size_t ir_capacity;
};

struct block_node;
typedef struct IR_Bytecode_Array IR_Bytecode_Array_t;
typedef IR_Bytecode_Array_t* IR_Bytecode_Array_ptr;

IR_Bytecode_Array_ptr ir_new(void);
void ir_free(const IR_Bytecode_Array_ptr);
Xen_int_t ir_emit(IR_Bytecode_Array_ptr, Xen_uint8_t, Xen_ulong_t,
                  Xen_Source_Address);
Xen_int_t ir_emit_jump(IR_Bytecode_Array_ptr, Xen_uint8_t, struct block_node*,
                       Xen_Source_Address);

#ifndef NDEBUG
void ir_print_block(struct block_node*, vm_Consts_ptr);
void ir_print(struct block_list*);
#endif

#endif
