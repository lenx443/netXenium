#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

Xen_String *Xen_String_From_CString(const char *cstring) {
  if (!cstring) { return NULL; }
  Xen_String *string = (Xen_String *)__instance_new(&Xen_String_Implement, NULL);
  if (!string) { return NULL; }
  for (const char *p = cstring; *p != '\0'; p++) {
    if (string->length >= string->capacity) {
      size_t new_capacity = string->capacity == 0 ? 4 : string->capacity * 2;
      char *new_mem = realloc(string->characters, new_capacity);
      if (!new_mem) { return NULL; }
      string->characters = new_mem;
      string->capacity = new_capacity;
    }
    string->characters[string->length++] = *p;
  }
  return string;
}

const char *Xen_String_As_CString(Xen_String *string) {
  if (!string) { return NULL; }
  return string->characters;
}
