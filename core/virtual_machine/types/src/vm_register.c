#include <stdlib.h>
#include <string.h>

#include "logs.h"
#include "vm_register.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

int vm_register_new(VM_Register *reg) {
  reg->reg = malloc(__REGISTER_CAPACITY__ * sizeof(reg_t));
  if (!reg->reg) {
    error("no hay memoria disponible");
    return 0;
  }
  reg->point_flag = calloc(__REGISTER_CAPACITY__, sizeof(uint8_t));
  if (!reg->point_flag) {
    error("no hay memoria disponible");
    free(reg->reg);
    return 0;
  }
  reg->capacity = __REGISTER_CAPACITY__;
  return 1;
}

void vm_register_clear(VM_Register *reg) {
  memset(reg->reg, 0, reg->capacity * sizeof(reg_t));
  memset(reg->point_flag, 0, reg->capacity);
}

void vm_register_free(VM_Register reg) {
  if (reg.reg) free(reg.reg);
  if (reg.point_flag) free(reg.point_flag);
}
