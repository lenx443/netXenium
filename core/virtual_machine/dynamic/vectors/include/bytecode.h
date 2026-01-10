#ifndef __BYTECODE_H__
#define __BYTECODE_H__

#include <stddef.h>
#include <stdint.h>

#include "bc_instruct.h"
#include "source_file.h"

struct Bytecode_Array {
  struct bc_Instruct* bc_array;
  size_t bc_size;
  size_t bc_capacity;
};

typedef struct Bytecode_Array Bytecode_Array_t;
typedef Bytecode_Array_t* Bytecode_Array_ptr;

Bytecode_Array_ptr bc_new(void);
void bc_clear(Bytecode_Array_ptr);
void bc_free(const Bytecode_Array_ptr);
void bc_emit(Bytecode_Array_ptr, uint8_t, uint8_t, Xen_Source_Address);

#ifndef NDEBUG
struct ProgramCode;
void bc_print(struct ProgramCode);
#endif

#endif
