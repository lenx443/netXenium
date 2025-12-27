#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "xen_alloc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_instance.h"
#include "xen_typedefs.h"

static void Xen_Number_Normalize(Xen_Number* n) {
  if (!n || !n->digits)
    return;

  if (n->size == 1 && n->digits[0] == 0) {
    n->sign = 0;
    n->scale = 0;
    return;
  }

  while (n->scale > 0) {
    uint64_t carry = 0;
    int divisible = 1;
    for (ssize_t i = n->size - 1; i >= 0; i--) {
      uint64_t cur = ((uint64_t)carry << 32) | n->digits[i];
      if (cur % 10 != 0)
        divisible = 0;
      carry = cur % 10;
    }

    if (!divisible)
      break;

    carry = 0;
    for (ssize_t i = n->size - 1; i >= 0; i--) {
      uint64_t cur = ((uint64_t)carry << 32) | n->digits[i];
      n->digits[i] = (uint32_t)(cur / 10);
      carry = cur % 10;
    }

    n->scale--;
  }

  while (n->size > 1 && n->digits[n->size - 1] == 0)
    n->size--;

  if (n->size == 1 && n->digits[0] == 0)
    n->sign = 0;
}

Xen_Instance* Xen_Number_Trunc(Xen_Instance* x) {
  if (!x)
    return NULL;

  Xen_Number* n = (Xen_Number*)x;
  Xen_Number* res = (Xen_Number*)Xen_Number_Copy(x);

  for (Xen_size_t i = n->scale; i < n->size; i++) {
    res->digits[i] = 0;
  }

  Xen_Number_Normalize(res);
  return (Xen_Instance*)res;
}

Xen_Instance* Xen_Number_Floor(Xen_Instance* x) {
  if (!x)
    return NULL;
  Xen_Instance* ONE = Xen_Number_From_CString("1", 10);

  Xen_Instance* integer_part = Xen_Number_Trunc(x);
  Xen_IGC_Push(integer_part);

  if (Xen_Number_Cmp(x, ONE) >= 0) {
    Xen_IGC_Pop();
    return integer_part;
  }

  Xen_Instance* diff = Xen_Number_Sub(x, integer_part);
  if (!Xen_Number_Is_Zero(diff)) {
    Xen_Instance* one = Xen_Number_From_CString("1", 10);
    Xen_IGC_Push(one);
    Xen_Instance* res = Xen_Number_Sub(integer_part, one);
    Xen_IGC_Pop();
    Xen_IGC_Pop();
    return res;
  }

  Xen_IGC_Pop();
  return integer_part;
}

static Xen_Number* Xen_Number_Mul_Pow10(Xen_Number* n, int32_t pow10) {
  if (!n)
    return NULL;
  if (pow10 == 0)
    return n;

  Xen_Number* result =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result)
    return NULL;

  result->sign = n->sign;
  result->scale = n->scale;

  Xen_Number* factor =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!factor)
    return NULL;

  factor->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
  factor->digits[0] = 1;
  factor->size = 1;
  factor->sign = 1;
  factor->scale = 0;
  Xen_IGC_Push((Xen_Instance*)factor);

  for (int i = 0; i < pow10; i++) {
    Xen_Instance* val = Xen_Number_From_Int(10);
    Xen_IGC_Push(val);
    Xen_Number* tmp = (Xen_Number*)Xen_Number_Mul((Xen_Instance*)factor, val);
    factor = tmp;
    Xen_IGC_XPOP(2);
    Xen_IGC_Push((Xen_Instance*)factor);
  }

  result = (Xen_Number*)Xen_Number_Mul((Xen_Instance*)n, (Xen_Instance*)factor);
  Xen_IGC_Pop();

  result->scale = n->scale + pow10;
  result->sign = n->sign;

  return result;
}

static Xen_Number* Xen_Number_BigDiv(Xen_Number* a, Xen_Number* b) {
  if (!a || !b)
    return NULL;
  if (Xen_Number_Is_Zero((Xen_Instance*)b))
    return NULL;

  Xen_Number* result =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result)
    return NULL;

  result->size = a->size;
  result->digits = (uint32_t*)calloc(result->size, sizeof(uint32_t));
  if (!result->digits) {
    Xen_Dealloc(result);
    return NULL;
  }

  uint32_t* remainder = (uint32_t*)calloc(a->size, sizeof(uint32_t));
  if (!remainder) {
    Xen_Dealloc(result->digits);
    Xen_Dealloc(result);
    return NULL;
  }
  memcpy(remainder, a->digits, a->size * sizeof(uint32_t));
  size_t rem_size = a->size;

  for (ssize_t i = (ssize_t)a->size - 1; i >= 0; i--) {
    uint64_t rem_high = ((size_t)i + 1 < rem_size) ? remainder[i + 1] : 0;
    uint64_t dividend = ((uint64_t)rem_high << 32) | remainder[i];

    uint32_t q = (uint32_t)(dividend / b->digits[0]);
    uint64_t sub = (uint64_t)q * b->digits[0];
    remainder[i] = (uint32_t)(dividend - sub);
    result->digits[i] = q;
  }

  size_t actual_size = result->size;
  while (actual_size > 1 && result->digits[actual_size - 1] == 0)
    actual_size--;
  result->size = actual_size;

  result->sign = a->sign * b->sign;
  result->scale = 0;

  Xen_Dealloc(remainder);
  return result;
}

static uint32_t BigMod_SingleLimb(const uint32_t* a, size_t a_size,
                                  uint32_t b) {
  uint64_t r = 0;
  for (ssize_t i = a_size - 1; i >= 0; i--) {
    uint64_t cur = (r << 32) | a[i];
    r = cur % b;
  }
  return (uint32_t)r;
}

static Xen_Number* Xen_Number_BigMod(Xen_Number* a, Xen_Number* b) {
  if (!a || !b || b->size == 0)
    return NULL;

  if (b->size == 1) {
    uint32_t rem_val = BigMod_SingleLimb(a->digits, a->size, b->digits[0]);

    Xen_Number* res = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    if (!res)
      return NULL;

    res->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    res->digits[0] = rem_val;
    res->size = (rem_val == 0) ? 0 : 1;
    res->sign = (rem_val == 0) ? 0 : 1;
    res->scale = 0;
    return res;
  }

  Xen_Number* remainder =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!remainder)
    return NULL;

  remainder->size = a->size;
  remainder->digits = (uint32_t*)calloc(remainder->size, sizeof(uint32_t));
  if (!remainder->digits) {
    Xen_Dealloc(remainder);
    return NULL;
  }
  memcpy(remainder->digits, a->digits, a->size * sizeof(uint32_t));
  remainder->sign = 1;
  remainder->scale = 0;

  Xen_Number* divisor = b;

  size_t n = remainder->size;
  size_t m = divisor->size;

  for (ssize_t i = n - 1; i >= (ssize_t)(m - 1); i--) {
    uint64_t r_hi = (i >= 1) ? remainder->digits[i - 1] : 0;
    uint64_t r_cur = ((uint64_t)remainder->digits[i] << 32) | r_hi;

    uint64_t q_hat = r_cur / divisor->digits[m - 1];
    if (q_hat > 0xFFFFFFFFu)
      q_hat = 0xFFFFFFFFu;

    uint64_t carry = 0;
    for (size_t j = 0; j < m; j++) {
      uint64_t sub = (uint64_t)divisor->digits[j] * q_hat + carry;
      uint64_t res =
          (uint64_t)remainder->digits[i - m + 1 + j] - (sub & 0xFFFFFFFFu);
      carry = (sub >> 32) + ((res >> 63) & 1);
      remainder->digits[i - m + 1 + j] = (uint32_t)res;
    }

    remainder->digits[i + 1 - m] -= (uint32_t)carry;
  }

  while (remainder->size > 1 && remainder->digits[remainder->size - 1] == 0)
    remainder->size--;

  return remainder;
}

Xen_bool_t Xen_IsNumber(Xen_Instance* inst) {
  return Xen_IMPL(inst) == xen_globals->implements->number;
}

int Xen_Number_Is_Zero(Xen_Instance* n_inst) {
  Xen_Number* n = (Xen_Number*)n_inst;
  for (size_t i = 0; i < n->size; i++) {
    if (n->digits[i] != 0)
      return false;
  }
  return true;
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

  Xen_Number* a_scaled = a;
  Xen_Number* b_scaled = b;
  int scale_diff = a->scale - b->scale;
  if (scale_diff > 0) {
    b_scaled = (Xen_Number*)Xen_Number_Mul_Pow10(b, scale_diff);
  } else if (scale_diff < 0) {
    a_scaled = (Xen_Number*)Xen_Number_Mul_Pow10(a, -scale_diff);
  }

  if (a_scaled->size > b_scaled->size) {
    return sign;
  }
  if (a_scaled->size < b_scaled->size) {
    return -sign;
  }

  for (Xen_ssize_t i = (Xen_ssize_t)a_scaled->size - 1; i >= 0; i--) {
    if (a_scaled->digits[i] > b_scaled->digits[i]) {
      return sign;
    }
    if (a_scaled->digits[i] < b_scaled->digits[i]) {
      return -sign;
    }
  }

  return 0;
}

Xen_Instance* Xen_Number_Copy(Xen_Instance* n_inst) {
  if (!n_inst)
    return NULL;
  Xen_Number* n = (Xen_Number*)n_inst;
  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!r)
    return NULL;
  r->digits = Xen_Alloc(n->size * sizeof(uint32_t));
  for (Xen_size_t i = 0; i < n->size; i++) {
    r->digits[i] = n->digits[i];
  }
  r->scale = n->scale;
  r->sign = n->sign;
  r->size = n->size;
  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_Div2(Xen_Instance* n_inst) {
  if (!n_inst)
    return NULL;

  Xen_Number* n = (Xen_Number*)n_inst;

  if (Xen_Number_Is_Zero(n_inst)) {
    Xen_Number* zero = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    return (Xen_Instance*)zero;
  }

  Xen_Number* res =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!res)
    return NULL;

  res->digits = (uint32_t*)Xen_Alloc(n->size * sizeof(uint32_t));
  memcpy(res->digits, n->digits, n->size * sizeof(uint32_t));
  res->size = n->size;
  res->sign = n->sign;

  uint32_t carry = 0;
  for (Xen_ssize_t i = (Xen_ssize_t)res->size - 1; i >= 0; i--) {
    uint64_t cur = ((uint64_t)carry << 32) | res->digits[i];
    res->digits[i] = (uint32_t)(cur / 2);
    carry = (uint32_t)(cur & 1);
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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z) {
    return NULL;
  }

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;
  z->scale = 0;

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
    } else {
      base = 10;
    }
  }

  if (base < 2 || base > 36) {
    return NULL;
  }

  const char* start = str;
  const char* end = str;
  const char* dot = NULL;

  for (;;) {
    unsigned char uc = (unsigned char)*end;

    if (uc == '.' && base == 10 && !dot) {
      dot = end;
      end++;
      continue;
    }

    int dv = Xen_Char_Digit_Value[uc];
    if (dv == -1 || dv >= base)
      break;

    end++;
  }

  if (end == start) {
    return NULL;
  }

  int32_t frac_len = 0;
  if (dot) {
    const char* frac_start = dot + 1;
    frac_len = (int32_t)(end - frac_start);
    if (frac_len < 0)
      frac_len = 0;
  }

  z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
  z->digits[0] = 0;
  z->size = 1;

  uint32_t base_d = (uint32_t)base;

  for (const char* p = start; p < end; p++) {
    if (*p == '.')
      continue;

    int dv = Xen_Char_Digit_Value[(unsigned char)*p];
    if (dv < 0 || dv >= base) {
      Xen_Dealloc(z->digits);
      z->digits = NULL;
      z->size = 0;
      return NULL;
    }

    uint64_t carry = (uint64_t)dv;
    for (size_t k = 0; k < z->size; k++) {
      uint64_t t = (uint64_t)z->digits[k] * base_d + carry;
      z->digits[k] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32;
    }

    while (carry) {
      z->digits =
          (uint32_t*)Xen_Realloc(z->digits, (z->size + 1) * sizeof(uint32_t));
      z->digits[z->size++] = (uint32_t)(carry & 0xFFFFFFFFu);
      carry >>= 32;
    }
  }

  if (dot && frac_len == 0) {
    uint64_t carry = 0;
    for (size_t k = 0; k < z->size; k++) {
      uint64_t t = (uint64_t)z->digits[k] * base_d + carry;
      z->digits[k] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32;
    }
    if (carry) {
      z->digits =
          (uint32_t*)Xen_Realloc(z->digits, (z->size + 1) * sizeof(uint32_t));
      z->digits[z->size++] = (uint32_t)carry;
    }
    z->scale = 1;
  } else {
    z->scale = frac_len;
  }

  int is_zero = (z->size == 1 && z->digits[0] == 0);
  z->sign = is_zero ? 0 : sign;

  while (z->size > 1 && z->digits[z->size - 1] == 0) {
    z->size--;
  }

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int32(int32_t value) {
  Xen_Number* z =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
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

  z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
  z->digits[0] = mag;
  z->size = 1;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Int64(int64_t value) {
  Xen_Number* z =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
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

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
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

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  uint64_t mag = (uint64_t)value;

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
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

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  unsigned long long mag = (unsigned long long)value;

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
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

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!z)
    return NULL;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE*)z;
  }

  int8_t sign = 1;
  unsigned long long mag = value;

  z->digits = (uint32_t*)Xen_Alloc(2 * sizeof(uint32_t));

  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE*)z;
}

Xen_INSTANCE* Xen_Number_From_Pointer(void* ptr) {
  return Xen_Number_From_LongLong((long long)(uintptr_t)ptr);
}

const char* Xen_Number_As_CString(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return NULL;

  if (n->sign == 0 || n->size == 0) {
    if (n->scale > 0) {
      char* z = Xen_Alloc(2 + n->scale);
      if (!z)
        return NULL;
      z[0] = '0';
      z[1] = '.';
      for (Xen_size_t i = 0; i < n->scale; i++)
        z[2 + i] = '0';
      z[2 + n->scale] = '\0';
      return z;
    } else {
      char* z = Xen_Alloc(2);
      if (!z)
        return NULL;
      z[0] = '0';
      z[1] = '\0';
      return z;
    }
  }

  uint32_t* temp = Xen_Alloc(n->size * sizeof(uint32_t));
  if (!temp)
    return NULL;
  memcpy(temp, n->digits, n->size * sizeof(uint32_t));
  size_t temp_size = n->size;

  char* buf = Xen_Alloc(temp_size * 10 + 1);
  if (!buf) {
    Xen_Dealloc(temp);
    return NULL;
  }

  size_t pos = 0;

  while (temp_size > 0) {
    uint64_t rem = 0;
    for (Xen_ssize_t i = (Xen_ssize_t)temp_size - 1; i >= 0; i--) {
      uint64_t cur = (rem << 32) | temp[i];
      temp[i] = (uint32_t)(cur / 10);
      rem = cur % 10;
    }
    buf[pos++] = (char)('0' + rem);
    while (temp_size > 0 && temp[temp_size - 1] == 0)
      temp_size--;
  }

  size_t digits = pos;
  size_t int_digits = (digits > (size_t)n->scale) ? (digits - n->scale) : 1;

  size_t total_len = (n->sign < 0 ? 1 : 0) + int_digits +
                     (n->scale > 0 ? 1 + n->scale : 0) + 1;

  char* str = Xen_Alloc(total_len);
  if (!str) {
    Xen_Dealloc(temp);
    Xen_Dealloc(buf);
    return NULL;
  }

  size_t idx = 0;

  if (n->sign < 0)
    str[idx++] = '-';

  if (digits <= (size_t)n->scale) {
    str[idx++] = '0';
  } else {
    for (Xen_ssize_t i = (Xen_ssize_t)digits - 1; i >= (Xen_ssize_t)n->scale;
         i--)
      str[idx++] = buf[i];
  }

  if (n->scale > 0) {
    str[idx++] = '.';
    for (Xen_ssize_t i = (Xen_ssize_t)n->scale - 1; i >= 0; i--) {
      if (i < (Xen_ssize_t)digits)
        str[idx++] = buf[i];
      else
        str[idx++] = '0';
    }
  }

  str[idx] = '\0';

  Xen_Dealloc(temp);
  Xen_Dealloc(buf);
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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
  for (Xen_ssize_t i = (Xen_ssize_t)n->size - 1; i >= 0; i--) {
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

Xen_uint8_t* Xen_Number_As_Bytes(Xen_Instance* inst, Xen_size_t* out_len) {
  Xen_Number* num = (Xen_Number*)Xen_Number_Copy(inst);
  while (num->size > 1 && num->digits[num->size - 1] == 0) {
    num->size--;
  }
  if (num->sign == 0 || (num->size == 1 && num->digits[0] == 0)) {
    *out_len = 1;
    Xen_uint8_t* out = Xen_Alloc(1);
    out[0] = 0;
    return out;
  }
  uint32_t msw = num->digits[num->size - 1];
  int msw_bytes = 0;
  if (msw >> 24) {
    msw_bytes = 4;
  } else if (msw >> 16) {
    msw_bytes = 3;
  } else if (msw >> 8) {
    msw_bytes = 2;
  } else {
    msw_bytes = 1;
  }
  *out_len = (num->size - 1) * 4 + msw_bytes;
  Xen_uint8_t* out = Xen_Alloc(*out_len);
  Xen_size_t k = 0;
  for (Xen_ssize_t i = (Xen_ssize_t)num->size - 1; i >= 0; i--) {
    Xen_uint32_t digit = num->digits[i];
    for (int b = 3; b >= 0; b--) {
      uint8_t byte = (digit >> (b * 8)) & 0xFF;
      if (k == 0 && byte == 0 && i == (Xen_ssize_t)num->size - 1) {
        continue;
      }
      out[k++] = byte;
    }
  }
  return out;
}

Xen_uint8_t* Xen_Number_As_Bytes_Flexible(Xen_Instance* inst,
                                          Xen_size_t* out_len, Xen_size_t align,
                                          int is_signed,
                                          Xen_uint8_t overflow_val,
                                          int big_endian) {
  Xen_Number* num = (Xen_Number*)Xen_Number_Copy(inst);
  while (num->size > 1 && num->digits[num->size - 1] == 0) {
    num->size--;
  }
  if (num->sign == 0 || (num->size == 1 && num->digits[0] == 0)) {
    *out_len = align ? align : 1;
    Xen_uint8_t* out = Xen_Alloc(*out_len);
    memset(out, 0x00, *out_len);
    return out;
  }
  uint32_t msw = num->digits[num->size - 1];
  Xen_size_t msw_bytes;
  if (msw >> 24)
    msw_bytes = 4;
  else if (msw >> 16)
    msw_bytes = 3;
  else if (msw >> 8)
    msw_bytes = 2;
  else
    msw_bytes = 1;
  Xen_size_t raw_len = (num->size - 1) * 4 + msw_bytes;

  if (align) {
    *out_len = (raw_len + (align - 1)) & ~(align - 1);
  } else {
    *out_len = raw_len;
  }

  Xen_uint8_t* out = Xen_Alloc(*out_len);
  memset(out, 0x00, *out_len);

  Xen_size_t k = big_endian ? (*out_len - raw_len) : 0;

  if (big_endian) {
    for (Xen_ssize_t i = (Xen_ssize_t)num->size - 1; i >= 0; i--) {
      Xen_uint32_t digit = num->digits[i];
      for (int b = 3; b >= 0; b--) {
        Xen_uint8_t byte = (digit >> (b * 8)) & 0xFF;
        if (k == (*out_len - raw_len) && byte == 0 &&
            i == (Xen_ssize_t)num->size - 1) {
          continue;
        }
        if ((is_signed && byte > INT8_MAX) ||
            (!is_signed && byte > UINT8_MAX)) {
          byte = overflow_val;
        }
        out[k++] = byte;
      }
    }
  } else {
    for (Xen_size_t i = 0; i < num->size; i++) {
      Xen_uint32_t digit = num->digits[i];
      for (int b = 0; b < 4; b++) {
        if (k >= raw_len)
          break;
        Xen_uint8_t byte = (digit >> (b * 8)) & 0xFF;
        if ((is_signed && byte > INT8_MAX) ||
            (!is_signed && byte > UINT8_MAX)) {
          byte = overflow_val;
        }
        out[k++] = byte;
      }
    }
  }

  return out;
}

Xen_Instance* Xen_Number_Mul(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if ((a->size == 1 && a->digits[0] == 0) ||
      (b->size == 1 && b->digits[0] == 0)) {
    Xen_Number* zero = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    if (!zero)
      return NULL;

    zero->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    zero->scale = 0;
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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result) {
    return NULL;
  }

  result->digits = res_digits;
  result->size = actual_size;
  result->sign = a->sign * b->sign;

  result->scale = a->scale + b->scale;

  Xen_Number_Normalize(result);

  return (Xen_Instance*)result;
}

#define XEN_DECIMAL_DIV_PRECISION 10

Xen_Instance* Xen_Number_Div(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero((Xen_Instance*)b))
    return NULL;
  if (Xen_Number_Is_Zero((Xen_Instance*)a)) {
    Xen_Number* zero = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    zero->scale = 0;
    return (Xen_Instance*)zero;
  }

  Xen_Number* remainder =
      (Xen_Number*)Xen_Number_Mod((Xen_Instance*)a, (Xen_Instance*)b);
  int exact = Xen_Number_Is_Zero((Xen_Instance*)remainder);

  Xen_Number* scaled;
  int precision = XEN_DECIMAL_DIV_PRECISION;

  if (exact) {
    scaled = a;
    precision = 0;
  } else {
    scaled = Xen_Number_Mul_Pow10(a, precision);
  }

  Xen_Number_Normalize(scaled);
  Xen_Number* quotient = Xen_Number_BigDiv(scaled, b);
  if (!quotient)
    return NULL;

  quotient->scale = a->scale - b->scale + precision;

  quotient->sign = a->sign * b->sign;

  Xen_Number_Normalize(quotient);

  return (Xen_Instance*)quotient;
}

Xen_Instance* Xen_Number_Mod(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero((Xen_Instance*)b))
    return NULL;
  if (Xen_Number_Is_Zero((Xen_Instance*)a)) {
    Xen_Number* zero = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    if (!zero)
      return NULL;

    zero->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    zero->scale = 0;
    return (Xen_Instance*)zero;
  }

  int scale_diff = a->scale - b->scale;
  Xen_Number* a_scaled = a;
  Xen_Number* b_scaled = b;

  if (scale_diff > 0) {
    b_scaled = Xen_Number_Mul_Pow10(b, scale_diff);
  } else if (scale_diff < 0) {
    a_scaled = Xen_Number_Mul_Pow10(a, -scale_diff);
  }

  Xen_Number* remainder = Xen_Number_BigMod(a_scaled, b_scaled);
  if (!remainder)
    return NULL;

  remainder->sign = a->sign;
  remainder->scale = (a->scale > b->scale) ? a->scale : b->scale;

  Xen_Number_Normalize(remainder);

  return (Xen_Instance*)remainder;
}

Xen_Instance* Xen_Number_Pow(Xen_Instance* base_inst, Xen_Instance* exp_inst) {
  if (!base_inst || !exp_inst)
    return NULL;

  Xen_Instance* one = Xen_Number_From_CString("1", 10);

  if (Xen_Number_Is_Zero(exp_inst))
    return one;

  Xen_Instance* result = Xen_Number_Mul(one, one);
  Xen_Instance* base = Xen_Number_Mul(base_inst, one);
  Xen_Instance* exp = Xen_Number_Mul(exp_inst, one);

  if (((Xen_Number*)exp)->scale == 0) {
    while (!Xen_Number_Is_Zero(exp)) {
      if (Xen_Number_Is_Odd(exp)) {
        Xen_Instance* tmp = Xen_Number_Mul(result, base);
        result = tmp;
      }

      Xen_Instance* tmp_base = Xen_Number_Mul(base, base);
      base = tmp_base;

      Xen_Instance* tmp_exp = Xen_Number_Div2(exp);
      exp = tmp_exp;
    }

    Xen_Number* res_num = (Xen_Number*)result;
    Xen_Number* base_num = (Xen_Number*)base_inst;

    if (Xen_Number_Is_Odd(exp_inst)) {
      res_num->sign = base_num->sign;
    } else {
      res_num->sign = 1;
    }

    Xen_Number_Normalize(res_num);
  } else {
    return NULL;
  }
  return result;
}

Xen_Instance* Xen_Number_Add(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (Xen_Number_Is_Zero(a_inst))
    return Xen_Number_Copy(b_inst);
  if (Xen_Number_Is_Zero(b_inst))
    return Xen_Number_Copy(a_inst);

  Xen_Number* a_scaled = a;
  Xen_Number* b_scaled = b;
  int scale_diff = a->scale - b->scale;

  if (scale_diff > 0) {
    Xen_IGC_Push((Xen_Instance*)b);
    b_scaled = Xen_Number_Mul_Pow10(b, scale_diff);
    Xen_IGC_Pop();
  } else if (scale_diff < 0) {
    Xen_IGC_Push((Xen_Instance*)a);
    a_scaled = Xen_Number_Mul_Pow10(a, -scale_diff);
    Xen_IGC_Pop();
  }

  if (a_scaled->sign != b_scaled->sign) {
    Xen_Number* res;
    if (a_scaled->sign < 0) {
      Xen_Number tmp = *a_scaled;
      tmp.sign = b_scaled->sign;
      res = (Xen_Number*)Xen_Number_Sub((Xen_Instance*)b_scaled,
                                        (Xen_Instance*)&tmp);
    } else {
      Xen_Number tmp = *b_scaled;
      tmp.sign = a_scaled->sign;
      res = (Xen_Number*)Xen_Number_Sub((Xen_Instance*)a_scaled,
                                        (Xen_Instance*)&tmp);
    }

    return (Xen_Instance*)res;
  }

  Xen_size_t max_size =
      (a_scaled->size > b_scaled->size ? a_scaled->size : b_scaled->size) + 1;
  uint32_t* res_digits = (uint32_t*)calloc(max_size, sizeof(uint32_t));
  if (!res_digits)
    return NULL;

  uint64_t carry = 0;
  Xen_size_t i;
  for (i = 0; i < max_size - 1; i++) {
    uint64_t av = (i < a_scaled->size) ? a_scaled->digits[i] : 0;
    uint64_t bv = (i < b_scaled->size) ? b_scaled->digits[i] : 0;
    uint64_t sum = av + bv + carry;
    res_digits[i] = (uint32_t)(sum & 0xFFFFFFFFu);
    carry = sum >> 32;
  }
  if (carry)
    res_digits[i++] = (uint32_t)carry;

  while (i > 1 && res_digits[i - 1] == 0)
    i--;

  Xen_Number* result =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result) {
    Xen_Dealloc(res_digits);
    return NULL;
  }

  result->digits = (uint32_t*)Xen_Alloc(i * sizeof(uint32_t));
  memcpy(result->digits, res_digits, i * sizeof(uint32_t));
  result->size = i;
  result->sign = a_scaled->sign;

  result->scale = (a->scale > b->scale) ? a->scale : b->scale;

  Xen_Number_Normalize(result);

  return (Xen_Instance*)result;
}

Xen_Instance* Xen_Number_Sub(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  int a_sign = (a->sign == 0) ? +1 : a->sign;
  int b_sign = (b->sign == 0) ? +1 : b->sign;

  if (a_sign != b_sign) {
    Xen_Number tmp = *b;
    tmp.sign = -b_sign;
    return Xen_Number_Add(a_inst, (Xen_Instance*)&tmp);
  }

  Xen_Number* a_scaled = a;
  Xen_Number* b_scaled = b;
  int scale_diff = a->scale - b->scale;
  if (scale_diff > 0) {
    b_scaled = Xen_Number_Mul_Pow10(b, scale_diff);
  } else if (scale_diff < 0) {
    a_scaled = Xen_Number_Mul_Pow10(a, -scale_diff);
  }

  int cmp = Xen_Number_Cmp((Xen_Instance*)a_scaled, (Xen_Instance*)b_scaled);
  if (cmp == 0) {
    Xen_Number* zero = (Xen_Number*)__instance_new(
        xen_globals->implements->number, nil, nil, 0);
    if (!zero)
      return NULL;
    zero->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    zero->digits[0] = 0;
    zero->size = 1;
    zero->sign = 0;
    zero->scale = 0;
    return (Xen_Instance*)zero;
  }

  const Xen_Number* minuend = (cmp >= 0) ? a_scaled : b_scaled;
  const Xen_Number* subtrahend = (cmp >= 0) ? b_scaled : a_scaled;
  int result_sign = (cmp >= 0) ? a_sign : -a_sign;

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
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result) {
    Xen_Dealloc(res_digits);
    return NULL;
  }

  result->digits = res_digits;
  result->size = res_size;
  result->sign = (res_size == 1 && res_digits[0] == 0) ? 0 : result_sign;

  result->scale = (a->scale > b->scale) ? a->scale : b->scale;

  Xen_Number_Normalize(result);

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
