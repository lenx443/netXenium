#include <stdlib.h>

#include "logs.h"
#include "vm_string_table.h"

vm_String_Table_ptr vm_string_table_new() {
  vm_String_Table_ptr table = malloc(sizeof(vm_String_Table));
  if (!table) {
    log_add(NULL, ERROR, "VM / String table", "No hay memoria disponible.");
    return NULL;
  }
  table->strings = NULL;
  table->size = 0;
  table->capacity = 0;
  return table;
}

int vm_string_table_add(vm_String_Table_ptr table, char *str) {
  if (!table) {
    log_add(NULL, ERROR, "VM / String table", "No se pudo agregar el elemento");
    log_add(NULL, ERROR, "VM / String table", "Tabla de cadenas vacía");
    return 0;
  }
  if (table->size >= table->capacity) {
    int new_capacity = (table->capacity == 0) ? 8 : table->capacity * 2;
    char **new_mem = realloc(table->strings, new_capacity * sizeof(char *));
    if (!new_mem) {
      log_add(NULL, ERROR, "VM / String table", "No se pudo agregar el elemento");
      log_add(NULL, ERROR, "VM / String table", "No hay memoria disponible");
      return 0;
    }
    table->strings = new_mem;
    table->capacity = new_capacity;
  }
  char *new_string = strdup(str);
  if (!new_string) {
    log_add(NULL, ERROR, "VM / String table", "No se pudo agregar el elemento");
    log_add(NULL, ERROR, "VM / String table", "No hay memoria disponible");
    return 0;
  }
  table->strings[table->size++] = new_string;
  return 1;
}

void vm_string_table_free(vm_String_Table_ptr table) {
  if (!table) return;
  for (int i = 0; i < table->size; i++)
    free(table->strings[i]);
  free(table->strings);
  free(table);
}
