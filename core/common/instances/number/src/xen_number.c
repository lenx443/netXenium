#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_typedefs.h"

int Xen_Number_Is_Zero(Xen_Instance* n_inst) {
  if (!n_inst)
    return 1;
  Xen_Number* n = (Xen_Number*)n_inst;
  return (n->size == 1 && n->digits[0] == 0) || n->sign == 0;
}

int Xen_Number_Is_Odd(Xen_Instance* n_inst) {
  if (!n_inst)
    return 0;
  Xen_Number* n = (Xen_Number*)n_inst;
  if (Xen_Number_Is_Zero(n_inst))
    return 0;
  return (n->digits[0] & 1) != 0;
}

int Xen_Number_Cmp(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return 0;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (a->sign > b->sign)
    return 1;
  if (a->sign < b->sign)
    return -1;

  int sign = a->sign;

  if (a->size > b->size)
    return sign;
  if (a->size < b->size)
    return -sign;

  for (Xen_ssize_t i = (Xen_ssize_t)a->size - 1; i >= 0; i--) {
    if (a->digits[i] > b->digits[i])
      return sign;
    if (a->digits[i] < b->digits[i])
      return -sign;
  }
  return 0;
}

Xen_Instance* Xen_Number_Div2(Xen_Instance* n_inst) {
  if (!n_inst)
    return NULL;

  Xen_Number* n = (Xen_Number*)n_inst;

  if (Xen_Number_Is_Zero(n_inst)) {
    Xen_Number* zero =
        (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!zero->digits) {
      Xen_DEL_REF(zero);
      return NULL;
    }
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  Xen_Number* res =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!res)
    return NULL;

  res->digits = (uint32_t*)malloc(n->size * sizeof(uint32_t));
  if (!res->digits) {
    Xen_DEL_REF(res);
    return NULL;
  }
  memcpy(res->digits, n->digits, n->size * sizeof(uint32_t));
  res->size = n->size;
  res->sign = n->sign;

  uint32_t carry = 0;
  for (ssize_t i = res->size - 1; i >= 0; i--) {
    uint64_t cur = ((uint64_t)carry << 32) | res->digits[i];
    res->digits[i] = (uint32_t)(cur / 2);
    carry = (uint32_t)(cur & 1); // resto
  }

  while (res->size > 1 && res->digits[res->size - 1] == 0)
    res->size--;

  return (Xen_Instance*)res;
}

Xen_INSTANCE* Xen_Number_From_CString(const char* cstring, int base) {
  if (!cstring) {
    return NULL;
  }

  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z) {
    return NULL;
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
    return NULL;
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
    return NULL;
  }

  z->digits = (uint32_t*)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return NULL;
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
      return NULL;
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
        return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
  }

  z->digits[0] = mag;
  z->size = 1;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int64(int64_t value) {
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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
  Xen_Number* z =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return NULL;
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
    return NULL;
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

Xen_Instance* Xen_Number_Mul(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if ((a->size == 1 && a->digits[0] == 0) ||
      (b->size == 1 && b->digits[0] == 0)) {
    Xen_Number* zero =
        (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!zero->digits) {
      Xen_DEL_REF(zero);
      return NULL;
    }
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  size_t res_size = a->size + b->size;
  uint32_t* res_digits = (uint32_t*)calloc(res_size, sizeof(uint32_t));
  if (!res_digits)
    return NULL;

  for (size_t i = 0; i < a->size; i++) {
    uint64_t carry = 0;
    for (size_t j = 0; j < b->size; j++) {
      uint64_t t = (uint64_t)a->digits[i] * (uint64_t)b->digits[j] +
                   res_digits[i + j] + carry;
      res_digits[i + j] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32;
    }
    size_t k = i + b->size;
    while (carry) {
      uint64_t t = (uint64_t)res_digits[k] + carry;
      res_digits[k] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32;
      k++;
    }
  }

  size_t actual_size = res_size;
  while (actual_size > 1 && res_digits[actual_size - 1] == 0)
    actual_size--;

  Xen_Number* result =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!result) {
    free(res_digits);
    return NULL;
  }

  result->digits = res_digits;
  result->size = actual_size;
  result->sign = (a->sign * b->sign);

  return (Xen_Instance*)result;
}

Xen_Instance* Xen_Number_Div(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero(b_inst))
    return NULL;

  if (Xen_Number_Is_Zero(a_inst)) {
    Xen_Number* zero =
        (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!zero->digits) {
      Xen_DEL_REF(zero);
      return NULL;
    }
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  Xen_size_t res_size = a->size;
  uint32_t* res_digits = (uint32_t*)calloc(res_size, sizeof(uint32_t));
  if (!res_digits)
    return NULL;

  uint32_t* remainder = (uint32_t*)calloc(a->size, sizeof(uint32_t));
  if (!remainder) {
    free(res_digits);
    return NULL;
  }
  memcpy(remainder, a->digits, a->size * sizeof(uint32_t));
  Xen_size_t rem_size = a->size;

  for (Xen_ssize_t i = (Xen_ssize_t)a->size - 1; i >= 0; i--) {
    uint64_t rem_high = ((Xen_size_t)i + 1 < rem_size) ? remainder[i + 1] : 0;
    uint64_t dividend = ((uint64_t)rem_high << 32) | remainder[i];
    uint32_t q = (uint32_t)(dividend / b->digits[0]);
    uint64_t sub = (uint64_t)q * b->digits[0];
    remainder[i] = (uint32_t)(dividend - sub);
    res_digits[i] = q;
  }

  size_t actual_size = res_size;
  while (actual_size > 1 && res_digits[actual_size - 1] == 0)
    actual_size--;

  Xen_Number* result =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!result) {
    free(res_digits);
    free(remainder);
    return NULL;
  }

  result->digits = res_digits;
  result->size = actual_size;
  result->sign = a->sign * b->sign;

  free(remainder);
  return (Xen_Instance*)result;
}

Xen_Instance* Xen_Number_Mod(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero(b_inst))
    return NULL;

  if (Xen_Number_Is_Zero(a_inst)) {
    Xen_Number* zero =
        (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!zero->digits) {
      Xen_DEL_REF(zero);
      return NULL;
    }
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  Xen_size_t rem_size = a->size;
  uint32_t* remainder = (uint32_t*)calloc(rem_size, sizeof(uint32_t));
  if (!remainder)
    return NULL;
  memcpy(remainder, a->digits, a->size * sizeof(uint32_t));

  if (b->size != 1) {
    free(remainder);
    return NULL;
  }

  uint32_t divisor = b->digits[0];
  uint64_t carry = 0;

  for (Xen_ssize_t i = rem_size - 1; i >= 0; i--) {
    uint64_t cur = (carry << 32) | remainder[i];
    uint64_t q = cur / divisor;
    carry = cur % divisor;
    remainder[i] = (uint32_t)q;
  }

  uint32_t remainder_val = (uint32_t)carry;

  Xen_Number* res =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!res) {
    free(remainder);
    return NULL;
  }

  res->digits = (uint32_t*)malloc(sizeof(uint32_t));
  if (!res->digits) {
    Xen_DEL_REF(res);
    free(remainder);
    return NULL;
  }

  res->digits[0] = remainder_val;
  res->size = 1;
  res->sign = a->sign;

  free(remainder);
  return (Xen_Instance*)res;
}

Xen_Instance* Xen_Number_Pow(Xen_Instance* base_inst, Xen_Instance* exp_inst) {
  if (!base_inst || !exp_inst)
    return NULL;

  Xen_Instance* zero = Xen_Number_From_CString("0", 10);
  Xen_Instance* one = Xen_Number_From_CString("1", 10);

  if (Xen_Number_Is_Zero(exp_inst)) {
    Xen_DEL_REF(zero);
    return one;
  }

  Xen_Instance* result = Xen_Number_Mul(one, one);
  Xen_Instance* base = Xen_Number_Mul(base_inst, one);
  Xen_Instance* exp = Xen_Number_Mul(exp_inst, one);

  while (!Xen_Number_Is_Zero(exp)) {
    if (Xen_Number_Is_Odd(exp)) {
      Xen_Instance* tmp = Xen_Number_Mul(result, base);
      Xen_DEL_REF(result);
      result = tmp;
    }

    Xen_Instance* tmp_base = Xen_Number_Mul(base, base);
    Xen_DEL_REF(base);
    base = tmp_base;

    Xen_Instance* tmp_exp = Xen_Number_Div2(exp);
    Xen_DEL_REF(exp);
    exp = tmp_exp;
  }

  Xen_DEL_REF(exp);
  Xen_DEL_REF(zero);
  Xen_DEL_REF(base);
  Xen_DEL_REF(one);

  ((Xen_Number*)result)->sign = ((Xen_Number*)base_inst)->sign;
  return result;
}

Xen_Instance* Xen_Number_Add(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero(a_inst))
    return Xen_Number_From_CString(Xen_Number_As_CString(b_inst), 10);
  if (Xen_Number_Is_Zero(b_inst))
    return Xen_Number_From_CString(Xen_Number_As_CString(a_inst), 10);

  if (a->sign != b->sign) {
    if (a->sign < 0) {
      Xen_Number tmp = *a;
      tmp.sign = b->sign;
      return Xen_Number_Sub(b_inst, (Xen_Instance*)&tmp);
    } else {
      Xen_Number tmp = *b;
      tmp.sign = a->sign;
      return Xen_Number_Sub(a_inst, (Xen_Instance*)&tmp);
    }
  }

  Xen_size_t max_size = (a->size > b->size ? a->size : b->size) + 1;
  uint32_t* res_digits = (uint32_t*)calloc(max_size, sizeof(uint32_t));
  if (!res_digits)
    return NULL;

  uint64_t carry = 0;
  Xen_size_t i;

  for (i = 0; i < max_size - 1; i++) {
    uint64_t av = (i < a->size) ? a->digits[i] : 0;
    uint64_t bv = (i < b->size) ? b->digits[i] : 0;
    uint64_t sum = av + bv + carry;
    res_digits[i] = (uint32_t)(sum & 0xFFFFFFFFu);
    carry = sum >> 32;
  }

  if (carry)
    res_digits[i++] = (uint32_t)carry;

  while (i > 1 && res_digits[i - 1] == 0)
    i--;

  Xen_Number* result =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!result) {
    free(res_digits);
    return NULL;
  }

  result->digits = (uint32_t*)malloc(i * sizeof(uint32_t));
  if (!result->digits) {
    Xen_DEL_REF(result);
    free(res_digits);
    return NULL;
  }

  memcpy(result->digits, res_digits, i * sizeof(uint32_t));
  result->size = i;
  result->sign = a->sign;

  free(res_digits);
  return (Xen_Instance*)result;
}

Xen_Instance* Xen_Number_Sub(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (a->sign != b->sign) {
    Xen_Number tmp = *b;
    tmp.sign = a->sign;
    return Xen_Number_Add(a_inst, (Xen_Instance*)&tmp);
  }

  int cmp = Xen_Number_Cmp(a_inst, b_inst);
  if (cmp == 0) {
    Xen_Number* zero =
        (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)malloc(sizeof(uint32_t));
    if (!zero->digits) {
      Xen_DEL_REF(zero);
      return NULL;
    }
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  const Xen_Number* minuend = a;
  const Xen_Number* subtrahend = b;
  int result_sign = a->sign;

  if (cmp < 0) {
    minuend = b;
    subtrahend = a;
    result_sign = -a->sign;
  }

  Xen_size_t res_size = minuend->size;
  uint32_t* res_digits = (uint32_t*)calloc(res_size, sizeof(uint32_t));
  if (!res_digits)
    return NULL;

  uint64_t borrow = 0;
  for (Xen_size_t i = 0; i < res_size; i++) {
    uint64_t a_digit = minuend->digits[i];
    uint64_t b_digit = (i < subtrahend->size) ? subtrahend->digits[i] : 0;
    uint64_t tmp = a_digit;
    if (tmp < b_digit + borrow) {
      tmp += ((uint64_t)1 << 32);
      res_digits[i] = (uint32_t)(tmp - b_digit - borrow);
      borrow = 1;
    } else {
      res_digits[i] = (uint32_t)(tmp - b_digit - borrow);
      borrow = 0;
    }
  }

  while (res_size > 1 && res_digits[res_size - 1] == 0)
    res_size--;

  Xen_Number* result =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!result) {
    free(res_digits);
    return NULL;
  }

  result->digits = res_digits;
  result->size = res_size;
  result->sign = result_sign;

  return (Xen_Instance*)result;
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
