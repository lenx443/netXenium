#ifndef __VM_REGISTER_H__
#define __VM_REGISTER_H__

#include <stddef.h>
#include <stdint.h>

typedef uintptr_t reg_t;

#ifndef __REGISTER_CAPACITY__
#define __REGISTER_CAPACITY__ 128
#endif

typedef struct {
  reg_t *reg;
  uint8_t *point_flag;
  size_t capacity;
} VM_Register;

int vm_register_new(VM_Register *);
void vm_register_clear(VM_Register *);
void vm_register_free(VM_Register);

#endif
