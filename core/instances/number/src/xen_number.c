#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"

/* Se asume que existen estas entidades en tu entorno:
   - extern const signed char Xen_Char_Digit_Value[256];
   - void * __instance_new(struct __Implement *impl, void *args);
   - void Xen_DEL_REF(void *obj);
   - extern struct __Implement Xen_Number_Implement;

   Estructura:
   struct Xen_Number_Instance {
     Xen_INSTANCE_HEAD;
     uint32_t *digits;
     size_t size;
     int8_t sign; // 0 = cero, +1 = positivo, -1 = negativo
   };
   typedef struct Xen_Number_Instance Xen_Number;
*/

Xen_INSTANCE *Xen_Number_From_CString(const char *cstring, int base) {
  if (!cstring) { return nil; }

  /* Crear la instancia primero (sin usar helpers adicionales) */
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) { return nil; }

  /* Inicializar a cero (por si el alloc no lo deja así) */
  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Saltar espacios iniciales */
  const char *str = cstring;
  while (*str && isspace((unsigned char)*str))
    str++;

  /* Signo */
  int8_t sign = 1;
  if (*str == '+') {
    str++;
  } else if (*str == '-') {
    sign = -1;
    str++;
  }

  /* Autodetección de base si base == 0 (similar a Python):
     0x/0X -> 16, 0o/0O -> 8, 0b/0B -> 2, 0 + más -> 8 (tradicional),
     en caso contrario -> 10. Aquí priorizamos prefijos explícitos. */
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
      base = 8; /* compat estilo C antiguo si empieza por 0 y hay más */
      str += 1;
    } else {
      base = 10;
    }
  }

  /* Validar rango de base */
  if (base < 2 || base > 36) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Detectar el rango [start, end) de dígitos válidos según la base */
  const char *start = str;
  const char *end = str;
  for (;;) {
    unsigned char uc = (unsigned char)*end;
    int dv = Xen_Char_Digit_Value[uc];
    if (dv == -1 || dv >= base) break;
    end++;
  }

  /* Debe haber al menos un dígito válido */
  if (end == start) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Reservar arreglo de dígitos inicial: valor 0 con un limb */
  z->digits = (uint32_t *)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }
  z->digits[0] = 0;
  z->size = 1;

  /* Acumular: z = z * base + c, en base interna 2^32 */
  const int len = (int)(end - start);
  uint32_t base_d = (uint32_t)base;

  for (int i = 0; i < len; i++) {
    unsigned char uc = (unsigned char)start[i];
    int dv = Xen_Char_Digit_Value[uc];
    /* Por robustez, revalidar (aunque ya validamos arriba) */
    if (dv < 0 || dv >= base) {
      /* literal inválido: liberar y abortar */
      free(z->digits);
      z->digits = NULL;
      z->size = 0;
      Xen_DEL_REF(z);
      return nil;
    }

    /* Multiplicar por base y sumar dv con propagación de carry */
    uint64_t carry = (uint64_t)dv;
    for (size_t k = 0; k < z->size; k++) {
      uint64_t t = (uint64_t)z->digits[k] * (uint64_t)base_d + carry;
      z->digits[k] = (uint32_t)(t & 0xFFFFFFFFu);
      carry = t >> 32; /* lo que excede 32 bits */
    }

    /* Si queda carry, agregar nuevos limbs (puede ser > 0) */
    while (carry != 0) {
      uint32_t limb = (uint32_t)(carry & 0xFFFFFFFFu);
      uint32_t *nd = (uint32_t *)realloc(z->digits, (z->size + 1) * sizeof(uint32_t));
      if (!nd) {
        /* fallo de memoria: liberar y abortar */
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

  /* Normalizar cero: si el valor es 0, sign = 0; si no, aplicar signo leído */
  int is_zero = (z->size == 1 && z->digits[0] == 0);
  if (is_zero) {
    z->sign = 0;
  } else {
    z->sign = sign;
  }

  /* Opcional: recortar ceros altos (normalización). Sin helpers,
     lo hacemos aquí mismo. */
  while (z->size > 1 && z->digits[z->size - 1] == 0) {
    z->size--;
    /* no reducimos memoria para evitar reallocs frecuentes; si lo deseas: */
    /* z->digits = realloc(z->digits, z->size * sizeof(uint32_t)); */
  }

  /* Saltar espacios finales (si quisieras devolver dónde terminó, aquí ‘end’) */
  /* while (*end && isspace((unsigned char)*end)) end++; */

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_Int32(int32_t value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Signo y magnitud */
  int8_t sign = 1;
  uint32_t mag;
  if (value < 0) {
    sign = -1;
    /* cuidado con INT32_MIN, donde -value desbordaría */
    mag = (uint32_t)(-(int64_t)value);
  } else {
    mag = (uint32_t)value;
  }

  /* Reservar un limb (máximo 32 bits) */
  z->digits = (uint32_t *)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  z->digits[0] = mag;
  z->size = 1;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_Int64(int64_t value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Manejar el caso cero explícitamente */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Signo */
  int8_t sign = 1;
  uint64_t mag; /* magnitud absoluta */
  if (value < 0) {
    sign = -1;
    mag = (uint64_t)(-value);
  } else {
    mag = (uint64_t)value;
  }

  /* Reservar suficiente espacio: un entero de 64 bits cabe en como mucho 2 limbs de 32
   * bits */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_Int(int value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Signo y magnitud */
  int8_t sign = 1;
  uint64_t mag; /* usamos 64 bits para cubrir cualquier rango de int */
  if (value < 0) {
    sign = -1;
    mag = (uint64_t)(-(int64_t)value); /* cuidado con INT_MIN */
  } else {
    mag = (uint64_t)value;
  }

  /* Reservar espacio: un int puede ocupar hasta 64 bits en teoría,
     por lo que máximo necesitamos 2 limbs de 32 bits */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_UInt(unsigned int value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* magnitud: como es unsigned, no hay signo que procesar */
  uint64_t mag = (uint64_t)value;

  /* Un unsigned int ocupa hasta 32 bits en la práctica,
     pero usamos 64 bits para portabilidad y manejamos hasta 2 limbs */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = +1; /* siempre positivo (a menos que sea 0) */

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_Long(long value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Signo y magnitud */
  int8_t sign = 1;
  unsigned long long mag; /* usamos 64 bits para cubrir cualquier rango de long */
  if (value < 0) {
    sign = -1;
    mag = (unsigned long long)(-(long long)value); /* cuidado con LONG_MIN */
  } else {
    mag = (unsigned long long)value;
  }

  /* Reservar espacio: un long puede ser 32 o 64 bits según la plataforma.
     En el peor caso (64 bits) necesitamos como mucho 2 limbs de 32 bits. */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_ULong(unsigned long value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Magnitud: como es unsigned long, no hay signo */
  unsigned long long mag = (unsigned long long)value;

  /* Reservar espacio: un unsigned long puede ser 32 o 64 bits según la plataforma.
     En el peor caso (64 bits) necesitamos como mucho 2 limbs de 32 bits. */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = 1; /* siempre positivo */

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_LongLong(long long value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Signo y magnitud */
  int8_t sign = 1;
  unsigned long long mag; /* usamos 64 bits para cubrir cualquier rango de long long */
  if (value < 0) {
    sign = -1;
    mag = (unsigned long long)(-(long long)value); /* cuidado con LLONG_MIN */
  } else {
    mag = (unsigned long long)value;
  }

  /* Reservar espacio: un long long es siempre de 64 bits,
     por lo que máximo necesitamos 2 limbs de 32 bits */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

Xen_INSTANCE *Xen_Number_From_ULongLong(unsigned long long value) {
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, nil, 0);
  if_nil_eval(z) return nil;

  z->digits = NULL;
  z->size = 0;
  z->sign = 0;

  /* Caso cero explícito */
  if (value == 0) {
    z->digits = (uint32_t *)malloc(sizeof(uint32_t));
    if (!z->digits) {
      Xen_DEL_REF(z);
      return nil;
    }
    z->digits[0] = 0;
    z->size = 1;
    z->sign = 0;
    return (Xen_INSTANCE *)z;
  }

  /* Como es unsigned, siempre es positivo */
  int8_t sign = 1;
  unsigned long long mag = value;

  /* Reservar espacio: un unsigned long long es siempre de 64 bits,
     por lo que máximo necesitamos 2 limbs de 32 bits */
  z->digits = (uint32_t *)malloc(2 * sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return nil;
  }

  /* Descomponer en base 2^32 */
  size_t n = 0;
  while (mag != 0) {
    z->digits[n++] = (uint32_t)(mag & 0xFFFFFFFFu);
    mag >>= 32;
  }

  z->size = n;
  z->sign = sign;

  return (Xen_INSTANCE *)z;
}

// Convierte un Xen_Number_Instance a un C-string decimal
const char *Xen_Number_As_CString(Xen_INSTANCE *inst) {
  Xen_Number *n = (Xen_Number *)inst;
  if (!n) return NULL;
  if (n->sign == 0 || n->size == 0) {
    char *zero = malloc(2);
    if (!zero) return NULL;
    zero[0] = '0';
    zero[1] = '\0';
    return zero;
  }

  // Copia temporal de los dígitos
  uint32_t *temp = malloc(n->size * sizeof(uint32_t));
  if (!temp) return NULL;
  memcpy(temp, n->digits, n->size * sizeof(uint32_t));
  size_t temp_size = n->size;

  // Array temporal para almacenar los dígitos decimales en orden inverso
  char *buf = malloc(temp_size * 10 * 10 + 2); // sobreestimación generosa
  if (!buf) {
    free(temp);
    return NULL;
  }

  size_t pos = 0;
  while (temp_size > 0) {
    uint64_t remainder = 0;
    // Dividir por 10, propagar carry
    for (ssize_t i = temp_size - 1; i >= 0; i--) {
      uint64_t cur = ((uint64_t)remainder << 32) | temp[i];
      temp[i] = (uint32_t)(cur / 10);
      remainder = cur % 10;
    }
    buf[pos++] = (char)('0' + remainder);

    // Reducir tamaño si el dígito más alto es 0
    while (temp_size > 0 && temp[temp_size - 1] == 0)
      temp_size--;
  }

  // Construir string final
  size_t len = pos + (n->sign < 0 ? 1 : 0) + 1; // +1 para '\0'
  char *str = malloc(len);
  if (!str) {
    free(temp);
    free(buf);
    return NULL;
  }

  size_t idx = 0;
  if (n->sign < 0) str[idx++] = '-';
  for (ssize_t i = pos - 1; i >= 0; i--)
    str[idx++] = buf[i];
  str[idx] = '\0';

  free(temp);
  free(buf);
  return str;
}

int32_t Xen_Number_As_Int32(Xen_INSTANCE *inst) {
  Xen_Number *n = (Xen_Number *)inst;
  if (!n) return 0;

  if (n->sign == 0 || n->size == 0) { return 0; }

  // Convertir acumulando en un entero de 64 bits para detectar overflow
  int64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];

    // Verificar overflow intermedio (si ya se pasó del rango de int32)
    if (value > (int64_t)INT32_MAX + 1) {
      // Si es positivo, ya se pasó
      if (n->sign > 0) return INT32_MAX;
      // Si es negativo, permitir hasta INT32_MIN
      if (value > (uint64_t)INT32_MAX + 1) return INT32_MIN;
    }
  }

  if (n->sign < 0) value = -value;

  // Clamp final al rango de int32
  if (value > INT32_MAX) return INT32_MAX;
  if (value < INT32_MIN) return INT32_MIN;

  return (int32_t)value;
}

int64_t Xen_Number_As_Int64(Xen_INSTANCE *inst) {
  Xen_Number *n = (Xen_Number *)inst;
  if (!n) return 0;

  if (n->sign == 0 || n->size == 0) { return 0; }

  // Si el número ocupa más de 64 bits, no cabe
  if (n->size > 2) { return (n->sign > 0) ? INT64_MAX : INT64_MIN; }

  // Reconstrucción en uint64_t
  uint64_t value = 0;
  for (ssize_t i = n->size - 1; i >= 0; i--) {
    value = (value << 32) | n->digits[i];
  }

  // Aplicar signo
  int64_t signed_value;
  if (n->sign < 0) {
    // Si es negativo, hay que asegurarse de no pasarse del límite
    if (value > (uint64_t)INT64_MAX + 1) {
      return INT64_MIN; // demasiado pequeño
    }
    signed_value = -(int64_t)value;
  } else {
    if (value > (uint64_t)INT64_MAX) {
      return INT64_MAX; // demasiado grande
    }
    signed_value = (int64_t)value;
  }

  return signed_value;
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
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
