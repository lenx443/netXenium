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
  string->length = strlen(cstring) + 1;
  string->characters = malloc(string->length);
  if (!string->characters) { return nil; }
  strncpy(string->characters, cstring, string->length);
  return (Xen_INSTANCE *)string;
}

Xen_Instance *Xen_String_From_Concat(Xen_Instance *s1, Xen_Instance *s2) {
  if (!s1 || !s2) { return nil; }
  size_t length = ((Xen_String *)s1)->length + ((Xen_String *)s2)->length + 1;
  char *buf = malloc(length);
  if (!buf) { return nil; }
  strcpy(buf, ((Xen_String *)s1)->characters);
  strcat(buf, ((Xen_String *)s2)->characters);
  buf[length - 1] = '\0';
  Xen_Instance *string = Xen_String_From_CString(buf);
  if_nil_eval(string) {
    free(buf);
    return nil;
  }
  free(buf);
  return string;
}

const char *Xen_String_As_CString(Xen_INSTANCE *string) {
  if (!string) { return NULL; }
  return ((Xen_String *)string)->characters;
}

char Xen_String_As_Char(Xen_Instance *string) {
  if (!string || !((Xen_String *)string)->characters ||
      ((Xen_String *)string)->length <= 0) {
    return '\0';
  }
  return *((Xen_String *)string)->characters;
}

unsigned long Xen_String_Hash(Xen_Instance *string_inst) {
  Xen_String *string = (Xen_String *)string_inst;
  char *temp = string->characters;
  unsigned long hash = 0x1505;
  int c;
  while ((c = *temp++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}
