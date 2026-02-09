#include <string.h>

#include "xen_cbuffer.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_string.h"
#include "xen_typedefs.h"

Xen_CBuffer* Xen_CBuffer_New(void) {
  Xen_CBuffer* cbuf = Xen_Alloc(sizeof(Xen_CBuffer));
  cbuf->buf = NULL;
  cbuf->length = cbuf->cap = 0;
  return cbuf;
}

Xen_CBuffer* Xen_CBuffer_New_From_CStr(Xen_c_string_t cstr) {
  Xen_CBuffer* cbuf = Xen_CBuffer_New();
  Xen_c_string_t c = cstr;
  while (*c) {
    Xen_CBuffer_Append_Char(cbuf, *c++);
  }
  return cbuf;
}

void Xen_CBuffer_Append_Char(Xen_CBuffer* cbuf, char c) {
  if (cbuf->length >= cbuf->cap) {
    Xen_size_t new_cap = (cbuf->cap) ? cbuf->cap * 2 : 4;
    cbuf->buf = Xen_Realloc(cbuf->buf, new_cap);
    cbuf->cap = new_cap;
  }
  cbuf->buf[cbuf->length++] = c;
}

void Xen_CBuffer_Append_CStr(Xen_CBuffer* cbuf, Xen_c_string_t cstr) {
  Xen_c_string_t c = cstr;
  while (*c) {
    Xen_CBuffer_Append_Char(cbuf, *c++);
  }
}

Xen_string_t Xen_CBuffer_As_CStr(Xen_CBuffer* cbuf) {
  Xen_string_t cstr = Xen_Alloc(cbuf->length + 1);
  for (Xen_size_t i = 0; i < cbuf->length; i++) {
    cstr[i] = cbuf->buf[i];
  }
  cstr[cbuf->length] = '\0';
  return cstr;
}

Xen_Instance* Xen_CBuffer_As_String(Xen_CBuffer* cbuf) {
  Xen_string_t cstr = Xen_Alloc(cbuf->length + 1);
  for (Xen_size_t i = 0; i < cbuf->length; i++) {
    cstr[i] = cbuf->buf[i];
  }
  cstr[cbuf->length] = '\0';
  return Xen_String_From_CString(cstr);
}

void Xen_CBuffer_Free(Xen_CBuffer *cbuf) {
  if (!cbuf) return;
  if (cbuf->buf) Xen_Dealloc(cbuf->buf);
  Xen_Dealloc(cbuf);
}
