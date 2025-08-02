#ifndef __VM_STRING_TABLE_H__
#define __VM_STRING_TABLE_H__

#include <stddef.h>

typedef struct {
  char **strings;
  size_t size, capacity;
} vm_String_Table;

typedef vm_String_Table *vm_String_Table_ptr;

vm_String_Table_ptr vm_string_table_new();
int vm_string_table_add(vm_String_Table_ptr, const char *);
void vm_string_table_clear(vm_String_Table_ptr);
void vm_string_table_free(vm_String_Table_ptr);

#endif
