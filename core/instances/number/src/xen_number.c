#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"

Xen_Number *Xen_Number_From_CString(const char *cstring, int base) {
  if (!cstring) { return NULL; }
  int8_t sign = 1;
  const char *start, *end;
  Xen_Number *z;
  const char *str = cstring;
  while (*str && isspace(*str))
    str++;
  if (*str == '+') {
    str++;
  } else if (*str == '-') {
    sign = -1;
    str++;
  }
  start = str;
  end = str;
  while (Xen_Char_Digit_Value[*end] < base && Xen_Char_Digit_Value[*end] != -1)
    end++;

  if (end == str) { return NULL; }

  int len = end - start;
  int str_i;
  uint32_t c;
  uint32_t base_d = (uint32_t)base;
  z = (Xen_Number *)__instance_new(&Xen_Number_Implement, NULL);
  z->digits = malloc(sizeof(uint32_t));
  *z->digits = 0;
  z->size = 1;

  for (str_i = 0; str_i < len; str_i++) {
    c = Xen_Char_Digit_Value[(unsigned char)str[str_i]];
    if (c >= base || c == -1) {
      Xen_DEL_REF(z);
      return NULL;
    }
  }

  z->sign = sign;
  return z;
}

const signed char Xen_Char_Digit_Value[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1,
    -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
