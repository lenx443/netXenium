#include "xen_cstrings.h"
#include "xen_alloc.h"
#include <stdint.h>

char* Xen_CString_From_Pointer(void* ptr) {
  char* buf = Xen_Alloc(2 + sizeof(void*) * 2 + 1);
  if (!buf)
    return NULL;

  static const char hex[] = "0123456789abcdef";
  uintptr_t v = (uintptr_t)ptr;
  char* p = buf;

  *p++ = '0';
  *p++ = 'x';

  int started = 0;
  for (int i = (sizeof(void*) * 2) - 1; i >= 0; i--) {
    uint8_t nibble = (v >> (i * 4)) & 0xF;
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
