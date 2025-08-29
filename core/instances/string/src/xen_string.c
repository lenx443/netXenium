#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

Xen_INSTANCE *Xen_String_From_CString(const char *cstring) {
  if (!cstring) { return nil; }
  Xen_String *string = (Xen_String *)__instance_new(&Xen_String_Implement, nil, 0);
  if_nil_eval(string) { return nil; }
  string->length = strlen(cstring);
  string->characters = malloc(string->length);
  if (!string->characters) { return nil; }
  strncpy(string->characters, cstring, string->length);
  return (Xen_INSTANCE *)string;
}

const char *Xen_String_As_CString(Xen_INSTANCE *string) {
  if (!string) { return NULL; }
  return ((Xen_String *)string)->characters;
}

const char Xen_String_As_Char(Xen_Instance *string) {
  if (!string || !((Xen_String *)string)->characters ||
      ((Xen_String *)string)->length <= 0) {
    return '\0';
  }
  return *((Xen_String *)string)->characters;
}
