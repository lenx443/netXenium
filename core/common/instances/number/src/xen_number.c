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
    n->sign = 1;
    n->scale = 0;
    return;
  }

  while (n->scale > 0 && (n->digits[0] % 10 == 0)) {
    uint64_t carry = 0;
    for (ssize_t i = n->size - 1; i >= 0; i--) {
      uint64_t cur = (carry << 32) | n->digits[i];
      n->digits[i] = (uint32_t)(cur / 10);
      carry = cur % 10;
    }
    n->scale--;
  }

  while (n->size > 1 && n->digits[n->size - 1] == 0)
    n->size--;

  if (n->size == 1 && n->digits[0] == 0)
    n->sign = 1;
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

static Xen_Number* Xen_Number_Mul_Pow10_2(Xen_Number* n, int32_t pow10) {
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
  result->size = n->size;
  result->digits = (uint32_t*)Xen_Alloc(result->size * sizeof(uint32_t));
  memcpy(result->digits, n->digits, result->size * sizeof(uint32_t));

  for (int32_t i = 0; i < pow10; i++) {
    uint64_t carry = 0;
    for (size_t j = 0; j < result->size; j++) {
      uint64_t cur = (uint64_t)result->digits[j] * 10 + carry;
      result->digits[j] = (uint32_t)(cur & 0xFFFFFFFF);
      carry = cur >> 32;
    }
    if (carry) {
      result->digits = (uint32_t*)realloc(result->digits, (result->size + 1) *
                                                              sizeof(uint32_t));
      result->digits[result->size++] = (uint32_t)carry;
    }
  }

  result->scale += pow10;
  return result;
}

static Xen_Number* Xen_Number_Mul_Pow10(Xen_Number* n, int32_t pow10) {
  if (!n)
    return NULL;
  if (pow10 == 0)
    return (Xen_Number*)Xen_Number_Copy((Xen_Instance*)n);

  Xen_Number* result =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!result)
    return NULL;

  result->digits = Xen_Alloc(n->size * sizeof(uint32_t));
  memcpy(result->digits, n->digits, n->size * sizeof(uint32_t));

  result->size = n->size;
  result->sign = n->sign;
  result->scale = n->scale - pow10;

  return result;
}

static Xen_Number* Xen_Number_BigDiv_u32(Xen_Number* a, uint32_t d) {
  if (!a || d == 0)
    return NULL;

  Xen_Number* q =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!q)
    return NULL;

  q->digits = Xen_ZAlloc(a->size, sizeof(uint32_t));
  if (!q->digits) {
    return NULL;
  }

  uint64_t rem = 0;

  for (ssize_t i = a->size - 1; i >= 0; i--) {
    uint64_t cur = (rem << 32) | a->digits[i];
    q->digits[i] = (uint32_t)(cur / d);
    rem = cur % d;
  }

  q->size = a->size;
  while (q->size > 1 && q->digits[q->size - 1] == 0)
    q->size--;

  q->sign = a->sign;
  q->scale = 0;

  return q;
}

static Xen_Number* Xen_Number_BigDiv(Xen_Number* u, Xen_Number* v) {
  if (!u || !v)
    return NULL;
  if (Xen_Number_Is_Zero((Xen_Instance*)v))
    return NULL;

  if (v->size == 1)
    return Xen_Number_BigDiv_u32(u, v->digits[0]);

  const uint64_t BASE = (1ULL << 32);

  size_t n = v->size;
  size_t m = u->size - n;

  Xen_Number* q =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!q)
    return NULL;

  q->digits = Xen_ZAlloc(m + 1, sizeof(uint32_t));
  if (!q->digits) {
    return NULL;
  }

  uint32_t* un = Xen_ZAlloc(u->size + 1, sizeof(uint32_t));
  uint32_t* vn = Xen_ZAlloc(v->size, sizeof(uint32_t));
  if (!un || !vn)
    goto fail;

  uint32_t shift = __builtin_clz(v->digits[n - 1]);
  uint64_t carry = 0;

  for (size_t i = 0; i < v->size; i++) {
    uint64_t cur = ((uint64_t)v->digits[i] << shift) | carry;
    vn[i] = (uint32_t)cur;
    carry = cur >> 32;
  }

  carry = 0;
  for (size_t i = 0; i < u->size; i++) {
    uint64_t cur = ((uint64_t)u->digits[i] << shift) | carry;
    un[i] = (uint32_t)cur;
    carry = cur >> 32;
  }
  un[u->size] = carry;

  for (ssize_t j = m; j >= 0; j--) {
    uint64_t u2 = ((uint64_t)un[j + n] << 32) | un[j + n - 1];
    uint64_t qhat = u2 / vn[n - 1];
    uint64_t rhat = u2 % vn[n - 1];

    if (qhat == BASE)
      qhat--;

    while (qhat * vn[n - 2] > ((rhat << 32) | un[j + n - 2])) {
      qhat--;
      rhat += vn[n - 1];
      if (rhat >= BASE)
        break;
    }

    int64_t borrow = 0;
    for (size_t i = 0; i < n; i++) {
      uint64_t p = qhat * vn[i];
      int64_t t = (int64_t)un[j + i] - (int64_t)(p & 0xFFFFFFFF) - borrow;
      un[j + i] = (uint32_t)t;
      borrow = (p >> 32) - (t >> 32);
    }

    int64_t t = (int64_t)un[j + n] - borrow;
    un[j + n] = (uint32_t)t;

    if (t < 0) {
      qhat--;
      uint64_t carryy = 0;
      for (size_t i = 0; i < n; i++) {
        uint64_t ty = (uint64_t)un[j + i] + vn[i] + carryy;
        un[j + i] = (uint32_t)ty;
        carryy = ty >> 32;
      }
      un[j + n] += carryy;
    }

    q->digits[j] = (uint32_t)qhat;
  }

  q->size = m + 1;
  while (q->size > 1 && q->digits[q->size - 1] == 0)
    q->size--;

  q->sign = u->sign * v->sign;
  q->scale = 0;

  Xen_Dealloc(un);
  Xen_Dealloc(vn);
  return q;

fail:
  Xen_Dealloc(un);
  Xen_Dealloc(vn);
  return NULL;
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
    res->size = 1;
    res->sign = 1;
    res->scale = 0;
    return res;
  }

  Xen_Number* remainder =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!remainder)
    return NULL;

  remainder->size = a->size;
  remainder->digits = (uint32_t*)Xen_ZAlloc(remainder->size, sizeof(uint32_t));
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
    b_scaled = (Xen_Number*)Xen_Number_Mul_Pow10_2(b, scale_diff);
  } else if (scale_diff < 0) {
    a_scaled = (Xen_Number*)Xen_Number_Mul_Pow10_2(a, -scale_diff);
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
    zero->sign = 1;
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
  z->sign = 1;
  z->scale = 0;

  const char* str = cstring;
  while (*str && isspace((unsigned char)*str))
    str++;

  if (*str == '+') {
    str++;
  } else if (*str == '-') {
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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
  z->sign = 1;

  if (value == 0) {
    z->digits = (uint32_t*)Xen_Alloc(sizeof(uint32_t));
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 1;
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

Xen_Instance* Xen_Number_From_Bytes(const Xen_uint8_t* bytes, Xen_size_t len,
                                    int is_signed, int big_endian) {
  Xen_Number* num =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  num->sign = 1;
  num->size = (len + 3) / 4;
  num->digits = Xen_Alloc(num->size * sizeof(uint32_t));
  memset(num->digits, 0, num->size * sizeof(uint32_t));

  for (Xen_size_t i = 0; i < len; i++) {
    Xen_size_t digit_index = i / 4;
    int byte_offset = i % 4;

    Xen_uint8_t b;
    if (big_endian) {
      b = bytes[len - 1 - i];
    } else {
      b = bytes[i];
    }

    num->digits[digit_index] |= ((uint32_t)b << (byte_offset * 8));
  }

  if (is_signed && (big_endian ? (bytes[0] & 0x80) : (bytes[len - 1] & 0x80))) {
    num->sign = -1;

    int carry = 1;
    for (Xen_size_t i = 0; i < num->size; i++) {
      num->digits[i] = ~num->digits[i];
      uint64_t temp = (uint64_t)num->digits[i] + carry;
      num->digits[i] = (uint32_t)temp;
      carry = (temp >> 32) & 0x1;
    }
  }

  while (num->size > 1 && num->digits[num->size - 1] == 0) {
    num->size--;
  }

  return (Xen_Instance*)num;
}

const char* Xen_Number_As_CString(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return NULL;

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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
  if (n->scale > 0) {
    char* end = str + strlen(str) - 1;

    while (end > str && *end == '0') {
      *end-- = '\0';
    }

    if (*end == '.') {
      *end = '\0';
    }
  }
  return str;
}

int32_t Xen_Number_As_Int32(Xen_INSTANCE* inst) {
  Xen_Number* n = (Xen_Number*)inst;
  if (!n)
    return 0;

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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

  if (n->sign == 0 || (n->size == 1 && n->digits[0] == 0) || n->size == 0) {
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
                                          int is_signed, int big_endian) {
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

  memset(out, (is_signed && num->sign < 0) ? 0xFF : 0x00, *out_len);

  Xen_uint8_t* temp = Xen_Alloc(raw_len);
  Xen_size_t k = 0;

  for (Xen_size_t i = 0; i < num->size; i++) {
    Xen_uint32_t digit = num->digits[i];
    for (int b = 0; b < 4; b++) {
      if (k >= raw_len)
        break;
      temp[k++] = (digit >> (b * 8)) & 0xFF;
    }
  }

  if (is_signed && num->sign < 0) {
    for (Xen_size_t i = 0; i < raw_len; i++) {
      temp[i] = ~temp[i];
    }
    for (Xen_size_t i = 0; i < raw_len; i++) {
      if (++temp[i] != 0)
        break;
    }
  }

  if (big_endian) {
    for (Xen_size_t i = 0; i < raw_len; i++) {
      out[*out_len - raw_len + i] = temp[i];
    }
  } else {
    for (Xen_size_t i = 0; i < raw_len; i++) {
      out[i] = temp[i];
    }
  }

  Xen_Dealloc(temp);
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
    zero->sign = 1;
    zero->scale = 0;
    return (Xen_Instance*)zero;
  }

  size_t res_size = a->size + b->size;
  uint32_t* res_digits = (uint32_t*)Xen_ZAlloc(res_size, sizeof(uint32_t));
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

#define XEN_DECIMAL_DIV_PRECISION 32

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
    scaled = Xen_Number_Mul_Pow10_2(a, precision);
  }

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
    zero->sign = 1;
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
  uint32_t* res_digits = (uint32_t*)Xen_ZAlloc(max_size, sizeof(uint32_t));
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
    zero->sign = 1;
    zero->scale = 0;
    return (Xen_Instance*)zero;
  }

  const Xen_Number* minuend = (cmp >= 0) ? a_scaled : b_scaled;
  const Xen_Number* subtrahend = (cmp >= 0) ? b_scaled : a_scaled;
  int result_sign = (cmp >= 0) ? a_sign : -a_sign;

  Xen_size_t res_size = minuend->size;
  uint32_t* res_digits = (uint32_t*)Xen_ZAlloc(res_size, sizeof(uint32_t));
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

static inline uint32_t sign_fill(Xen_Number* n) {
  return n->sign == -1 ? UINT32_MAX : 0;
}

static inline void bnormalize(Xen_Number* x) {
  uint32_t fill = (x->sign == -1) ? UINT32_MAX : 0;
  while (x->size > 1 && x->digits[x->size - 1] == fill) {
    uint32_t msb = x->digits[x->size - 2] >> 31;
    if (msb == ((x->sign == -1) ? 1 : 0))
      x->size--;
    else
      break;
  }
  x->sign = ((x->digits[x->size - 1] >> 31) & 1) ? -1 : 1;
  if (x->size == 1 && x->digits[0] == 0) {
    x->sign = 1;
  }
}

Xen_Instance* Xen_Number_BAnd(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (a->scale > 0 || b->scale > 0) {
    return NULL;
  }

  Xen_size_t size = a->size < b->size ? b->size : a->size;

  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  r->digits = Xen_ZAlloc(size, sizeof(uint32_t));
  r->size = size;

  for (Xen_size_t i = 0; i < size; i++) {
    int32_t wa = (i < a->size) ? a->digits[i] : sign_fill(a);
    int32_t wb = (i < b->size) ? b->digits[i] : sign_fill(b);
    r->digits[i] = wa & wb;
  }
  bnormalize(r);
  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_BXor(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (a->scale > 0 || b->scale > 0) {
    return NULL;
  }

  Xen_size_t size = a->size < b->size ? b->size : a->size;

  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  r->digits = Xen_ZAlloc(size, sizeof(uint32_t));
  r->size = size;

  for (Xen_size_t i = 0; i < size; i++) {
    int32_t wa = (i < a->size) ? a->digits[i] : sign_fill(a);
    int32_t wb = (i < b->size) ? b->digits[i] : sign_fill(b);
    r->digits[i] = wa ^ wb;
  }
  bnormalize(r);
  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_BOr(Xen_Instance* a_inst, Xen_Instance* b_inst) {
  if (!a_inst || !b_inst)
    return NULL;

  Xen_Number* a = (Xen_Number*)a_inst;
  Xen_Number* b = (Xen_Number*)b_inst;

  if (a->scale > 0 || b->scale > 0) {
    return NULL;
  }

  Xen_size_t size = a->size < b->size ? b->size : a->size;

  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  r->digits = Xen_ZAlloc(size, sizeof(uint32_t));
  r->size = size;

  for (Xen_size_t i = 0; i < size; i++) {
    int32_t wa = (i < a->size) ? a->digits[i] : sign_fill(a);
    int32_t wb = (i < b->size) ? b->digits[i] : sign_fill(b);
    r->digits[i] = wa | wb;
  }
  bnormalize(r);
  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_BNot(Xen_Instance* n_inst) {
  if (!n_inst)
    return NULL;

  Xen_Number* n = (Xen_Number*)n_inst;

  if (n->scale > 0)
    return NULL;

  Xen_Instance* one = Xen_Number_From_Int(1);

  Xen_Number* r = (Xen_Number*)Xen_Number_Add((Xen_Instance*)n, one);

  if (n->sign == 1) {
    r->sign = -1;
  } else if (n->sign == -1) {
    r->sign = 1;
  } else {
    return NULL;
  }

  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_SHL(Xen_Instance* n_inst, Xen_uint64_t nbits) {
  Xen_Number* n = (Xen_Number*)n_inst;

  if (n->scale > 0) {
    return NULL;
  }

  Xen_size_t word_shift = nbits / 32;
  Xen_size_t bit_shift = nbits % 32;

  Xen_size_t size = n->size + word_shift + (bit_shift ? 1 : 0);

  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  r->digits = Xen_ZAlloc(size, sizeof(uint32_t));
  r->size = size;
  r->sign = n->sign;
  for (Xen_size_t i = 0; i < n->size; i++) {
    Xen_size_t j = i + word_shift;
    r->digits[j] |= n->digits[i] << bit_shift;
    if (bit_shift && j + 1 < size) {
      r->digits[j + 1] |= n->digits[i] >> (32 - bit_shift);
    }
  }
  bnormalize(r);
  return (Xen_Instance*)r;
}

Xen_Instance* Xen_Number_SHR(Xen_Instance* n_inst, Xen_uint64_t nbits) {
  Xen_Number* n = (Xen_Number*)n_inst;

  if (n->scale > 0) {
    return NULL;
  }

  Xen_size_t word_shift = nbits / 32;
  Xen_size_t bit_shift = nbits % 32;

  if (word_shift >= n->size) {
    return Xen_Number_From_UInt((n->sign == -1) ? UINT32_MAX : 0);
  }
  Xen_size_t size = n->size - word_shift;

  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  r->digits = Xen_ZAlloc(size, sizeof(uint32_t));
  r->size = size;
  r->sign = n->sign;

  for (Xen_size_t i = 0; i < size; i++) {
    Xen_size_t j = i + word_shift;
    r->digits[i] = n->digits[j] >> bit_shift;
    if (bit_shift && j + 1 < n->size) {
      r->digits[i] |= n->digits[j + 1] << (32 - bit_shift);
    }
  }

  if (n->sign == -1 && bit_shift) {
    r->digits[size - 1] |= ~((1U << (32 - bit_shift)) - 1);
  }
  bnormalize(r);
  return (Xen_Instance*)r;
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
