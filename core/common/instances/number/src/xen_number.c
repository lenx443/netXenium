#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"

Xen_INSTANCE* Xen_Number_From_CString(const char* cstring, int base) {
  if (!cstring) {
    return nil;
  }

  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) {
    return nil;
  }

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  const char* str = cstring;
  while (*str && isspace((unsigned char)*str))
    str++;

  int8_t sign = 1;
  if (*str == '+') {
    str++;
  } else if (*str == '-') {
    sign = -1;
    str++;
  }

  if (base == 0) {
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
      base = 16;
      str += 2;
    } else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
      base = 2;
      str += 2;
    } else if (str[0] == '0' && (str[1] == 'o' || str[1] == 'O')) {
      base = 8;
      str += 2;
    } else if (str[0] == '0' && isalnum((unsigned char)str[1])) {
      base = 8;
      str += 1;
    } else {
      base = 10;
    }
  }

  if (base < 2 || base > 36) {
    Xen_DEL_REF(z);
    return nil;
  }

  const char* start = str;
  const char* end = str;
  for (;;) {
    unsigned char uc = (unsigned char)*end;
    int dv = Xen_Char_Digit_Value[uc];
    if (dv == -1 || dv >= base)
      break;
    end++;
  }

  if (end == start) {
    Xen_DEL_REF(z);
    return nil;
  }

  z->digits = (uint32_t*)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }
  z->digits[0] = 0;
  z->size = 1;

  const int len = (int)(end - start);
  uint32_t base_d = (uint32_t)base;

  for (int i = 0; i < len; i++) {
    unsigned char uc = (unsigned char)start[i];
    int dv = Xen_Char_Digit_Value[uc];
    if (dv < 0 || dv >= base) {
      free(z->digits);
      z->digits = NULL;
      z->size = 0;
      Xen_DEL_REF(z);
      return nil;
    }

    uint64_t carry = (uint64_t)dv;
    for (size_t k = 0; k < z->size; k++) {
      uint64_t t = (uint64_t)z->digits[k] * (uint64_t)base_d + carry;
      z->digits[k] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32;
    }

    while (carry != 0) {
      uint32_t limb = (uint32_t)(carry & 0xFFFFFFFFu);
      uint32_t* nd =
          (uint32_t*)realloc(z->digits, (z->size + 1) * sizeof(uint32_t));
      if (!nd) {
        free(z->digits);
        z->digits = NULL;
        z->size = 0;
        Xen_DEL_REF(z);
        return nil;
      }
      z->digits = nd;
      z->digits[z->size] = limb;
      z->size += 1;
      carry >>= 32;
    }
  }

  int is_zero = (z->size == 1 && z->digits[0] == 0);
  if (is_zero) {
    z->sign = 0;
  } else {
    z->sign = sign;
  }

  while (z->size > 1 && z->digits[z->size - 1] == 0) {
    z->size--;
  }

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int32(int32_t value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  uint32_t mag;
  if (value < 0) {
    sign = -1;
    mag = (uint32_t)(-(int64_t)value);
  } else {
    mag = (uint32_t)value;
  }

  z->digits = (uint32_t*)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  z->digits[0] = mag;
  z->size = 1;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int64(int64_t value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  uint64_t mag;
  if (value < 0) {
    sign = -1;
    mag = (uint64_t)(-value);
  } else {
    mag = (uint64_t)value;
  }

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int(int value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  uint64_t mag;
  if (value < 0) {
    sign = -1;
    mag = (uint64_t)(-(int64_t)value);
  } else {
    mag = (uint64_t)value;
  }

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_UInt(unsigned int value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  uint64_t mag = (uint64_t)value;

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = +1;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Long(long value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  unsigned long long mag;
  if (value < 0) {
    sign = -1;
    mag = (unsigned long long)(-(long long)value);
  } else {
    mag = (unsigned long long)value;
  }

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_ULong(unsigned long value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  unsigned long long mag = (unsigned long long)value;

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = 1;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_LongLong(long long value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  unsigned long long mag;
  if (value < 0) {
    sign = -1;
    mag = (unsigned long long)(-(long long)value);
  } else {
    mag = (unsigned long long)value;
  }

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_ULongLong(unsigned long long value) {
  Xen_Number* z = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  unsigned long long mag = value;

  z->digits = (uint32_t*)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

const char* Xen_Number_As_CString(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return NULL;
  if (n->sign == 0 || n->size == 0) {
    char* zero = malloc(2);
    if (!zero)
      return NULL;
    zero[0] = '0';
    zero[1] = '\0';
    return zero;
  }

  uint32_t* temp = malloc(n->size * sizeof(uint32_t));
  if (!temp)
    return NULL;
  memcpy(temp, n->digits, n->size * sizeof(uint32_t));
  size_t temp_size = n->size;

  char* buf = malloc(temp_size * 10 * 10 + 2);
  if (!buf) {
    free(temp);
    return NULL;
  }

  size_t pos = 0;
  while (temp_size > 0) {
    uint64_t remainder = 0;
    for (ssize_t i = temp_size - 1; i >= 0; i--) {
      uint64_t cur = ((uint64_t)remainder << 32) | temp[i];
      temp[i] = (uint32_t)(cur / 10);
      remainder = cur % 10;
    }
    buf[pos++] = (char)('0' + remainder);

    while (temp_size > 0 && temp[temp_size - 1] == 0)
      temp_size--;
  }

  size_t len = pos + (n->sign < 0 ? 1 : 0) + 1;
  char* str = malloc(len);
  if (!str) {
    free(temp);
    free(buf);
    return NULL;
  }

  size_t idx = 0;
  if (n->sign < 0)
    str[idx++] = '-';
  for (ssize_t i = pos - 1; i >= 0; i--)
    str[idx++] = buf[i];
  str[idx] = '\0';

  free(temp);
  free(buf);
  return str;
}

int32_t Xen_Number_As_Int32(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || n->size == 0) {
    return 0;
  }

  int64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];

    if (value > (int64_t)INT32_MAX + 1) {
      if (n->sign > 0)
        return INT32_MAX;
      if (value > (int64_t)INT32_MAX + 1)
        return INT32_MIN;
    }
  }

  if (n->sign < 0)
    value = -value;

  if (value > INT32_MAX)
    return INT32_MAX;
  if (value < INT32_MIN)
    return INT32_MIN;

  return (int32_t)value;
}

int64_t Xen_Number_As_Int64(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || n->size == 0) {
    return 0;
  }

  if (n->size > 2) {
    return (n->sign > 0) ? INT64_MAX : INT64_MIN;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  int64_t signed_value;
  if (n->sign < 0) {
    if (value > (uint64_t)INT64_MAX + 1) {
      return INT64_MIN;
    }
    signed_value = -(int64_t)value;
  } else {
    if (value > (uint64_t)INT64_MAX) {
      return INT64_MAX;
    }
    signed_value = (int64_t)value;
  }

  return signed_value;
}

int Xen_Number_As_Int(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    if (value > (uint64_t)INT_MAX + 1ULL) {
      return INT_MIN;
    }
    return -(int)value;
  } else {
    if (value > (uint64_t)INT_MAX) {
      return INT_MAX;
    }
    return (int)value;
  }
}

unsigned int Xen_Number_As_UInt(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    return 0;
  }

  if (value > (uint64_t)UINT_MAX) {
    return UINT_MAX;
  }

  return (unsigned int)value;
}

long Xen_Number_As_Long(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    if (value > (uint64_t)LONG_MAX + 1ULL) {
      return LONG_MIN;
    }
    return -(long)value;
  } else {
    if (value > (uint64_t)LONG_MAX) {
      return LONG_MAX;
    }
    return (long)value;
  }
}

unsigned long Xen_Number_As_ULong(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    return 0;
  }

  if (value > (uint64_t)ULONG_MAX) {
    return ULONG_MAX;
  }

  return (unsigned long)value;
}

long long Xen_Number_As_LongLong(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    if (value > (uint64_t)LLONG_MAX + 1ULL) {
      return LLONG_MIN;
    }
    return -(long long)value;
  } else {
    if (value > (uint64_t)LLONG_MAX) {
      return LLONG_MAX;
    }
    return (long long)value;
  }
}

unsigned long long Xen_Number_As_ULongLong(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->size == 0) {
    return 0;
  }

  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  if (n->sign < 0) {
    return 0;
  }

  if (value > (uint64_t)ULLONG_MAX) {
    return ULLONG_MAX;
  }

  return (unsigned long long)value;
}

const signed char Xen_Char_Digit_Value[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,
    9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
