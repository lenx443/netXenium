#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

Xen_String *Xen_String_From_CString(const char *cstring) {
  if (!cstring) { return NULL; }
  Xen_String *string = (Xen_String *)__instance_new(&Xen_String_Implement, NULL, 0);
  if (!string) { return NULL; }
  string->length = strlen(cstring);
  string->characters = malloc(string->length);
  if (!string->characters) { return NULL; }
  strncpy(string->characters, cstring, string->length);
  return string;
}

const char *Xen_String_As_CString(Xen_String *string) {
  if (!string) { return NULL; }
  return string->characters;
}
