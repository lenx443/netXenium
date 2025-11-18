#include <stdint.h>
#include <string.h>

#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_typedefs.h"

Xen_string_t Xen_CString_From_Pointer(void* ptr) {
  Xen_string_t buf = Xen_Alloc(2 + sizeof(void*) * 2 + 1);
  if (!buf)
    return NULL;

  static const char hex[] = "0123456789abcdef";
  Xen_uintptr_t v = (Xen_uintptr_t)ptr;
  char* p = buf;

  *p++ = '0';
  *p++ = 'x';

  int started = 0;
  for (int i = (sizeof(void*) * 2) - 1; i >= 0; i--) {
    Xen_uint8_t nibble = (v >> (i * 4)) & 0xF;
    if (nibble != 0 || started) {
      *p++ = hex[nibble];
      started = 1;
    }
  }

  if (!started)
    *p++ = '0';

  *p = '\0';
  return buf;
}

Xen_string_t Xen_CString_As_Raw(Xen_c_string_t str) {
  if (!str)
    return NULL;
  Xen_size_t capacity = 64;
  Xen_string_t out = (Xen_string_t)Xen_Alloc(capacity);
  if (!out)
    return NULL;
  Xen_size_t len = 0;
  const unsigned char* p = (const unsigned char*)str;
  while (*p) {
    char c = *p;
    Xen_c_string_t escape = NULL;
    Xen_size_t esc_len = 0;
    char buf[4];
    switch (c) {
    case '\\':
      escape = "\\\\";
      esc_len = 2;
      break;
    case '\"':
      escape = "\\\"";
      esc_len = 2;
      break;
    case '\'':
      escape = "\\\'";
      esc_len = 2;
      break;
    case '\n':
      escape = "\\n";
      esc_len = 2;
      break;
    case '\r':
      escape = "\\r";
      esc_len = 2;
      break;
    case '\t':
      escape = "\\t";
      esc_len = 2;
      break;
    case '\b':
      escape = "\\b";
      esc_len = 2;
      break;
    case '\f':
      escape = "\\f";
      esc_len = 2;
      break;
    case '\v':
      escape = "\\v";
      esc_len = 2;
      break;
    case '\a':
      escape = "\\a";
      esc_len = 2;
      break;
    default:
      if (c < 32 || c >= 127) {
        static const char HEX[] = "0123456789ABCDEF";
        buf[0] = '\\';
        buf[1] = 'x';
        buf[2] = HEX[(c >> 4) & 0xF];
        buf[3] = HEX[c & 0xF];
        escape = buf;
        esc_len = 4;
      } else {
        buf[0] = (char)c;
        escape = buf;
        esc_len = 1;
      }
      break;
    }
    if (len + esc_len + 1 > capacity) {
      Xen_size_t new_capacity = (capacity * 2) + esc_len + 1;
      char* tmp = Xen_Realloc(out, new_capacity);
      if (!tmp) {
        Xen_Dealloc(out);
        return NULL;
      }
      out = tmp;
      capacity = new_capacity;
    }
    for (size_t i = 0; i < esc_len; i++)
      out[len++] = escape[i];
    p++;
  }

  out[len] = '\0';
  return out;
}

Xen_size_t Xen_CString_Len(Xen_c_string_t str) {
  if (!str) {
    return 0;
  }
  const char* p = str;
  while (*p != '\0') {
    p++;
  }
  return (Xen_size_t)(p - str);
}

Xen_string_t Xen_CString_Dup(Xen_c_string_t str) {
  if (!str) {
    return NULL;
  }
  Xen_size_t len = Xen_CString_Len(str) + 1;
  Xen_string_t dup = (Xen_string_t)Xen_Alloc(len);
  if (!dup) {
    return NULL;
  }
  memcpy(dup, str, len);
  return dup;
}
