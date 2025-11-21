#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

Xen_INSTANCE* Xen_String_From_CString(const char* cstring) {
  if (!cstring) {
    return NULL;
  }
  Xen_String* string =
      (Xen_String*)__instance_new(&Xen_String_Implement, nil, nil, 0);
  if (!string) {
    return NULL;
  }
  string->__size = Xen_CString_Len(cstring);
  string->characters = Xen_Alloc(string->__size + 1);
  strncpy(string->characters, cstring, string->__size + 1);
  return (Xen_INSTANCE*)string;
}

Xen_Instance* Xen_String_From_Char(char c) {
  Xen_String* string =
      (Xen_String*)__instance_new(&Xen_String_Implement, nil, nil, 0);
  if (!string) {
    return NULL;
  }
  string->__size = 1;
  string->characters = Xen_Alloc(string->__size + 1);
  string->characters[0] = c;
  string->characters[1] = '\0';
  return (Xen_Instance*)string;
}

Xen_Instance* Xen_String_From_Concat(Xen_Instance* s1, Xen_Instance* s2) {
  if (!s1 || !s2) {
    return NULL;
  }
  size_t length = ((Xen_String*)s1)->__size + ((Xen_String*)s2)->__size + 1;
  char* buf = Xen_Alloc(length);
  if (!buf) {
    return NULL;
  }
  strcpy(buf, ((Xen_String*)s1)->characters);
  strcat(buf, ((Xen_String*)s2)->characters);
  buf[length - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buf);
  if (!string) {
    Xen_Dealloc(buf);
    return NULL;
  }
  Xen_Dealloc(buf);
  return string;
}

const char* Xen_String_As_CString(Xen_INSTANCE* string) {
  if (!string) {
    return NULL;
  }
  return ((Xen_String*)string)->characters;
}

char Xen_String_As_Char(Xen_Instance* string) {
  if (!string || !((Xen_String*)string)->characters ||
      ((Xen_String*)string)->__size <= 0) {
    return '\0';
  }
  return *((Xen_String*)string)->characters;
}

unsigned long Xen_String_Hash(Xen_Instance* string_inst) {
  Xen_String* string = (Xen_String*)string_inst;
  char* temp = string->characters;
  unsigned long hash = 0x1505;
  int c;
  while ((c = *temp++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}
