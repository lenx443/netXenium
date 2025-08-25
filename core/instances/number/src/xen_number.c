#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "instance.h"
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

Xen_Number *Xen_Number_From_CString(const char *cstring, int base) {
  if (!cstring) { return NULL; }

  /* Crear la instancia primero (sin usar helpers adicionales) */
  Xen_Number *z = (Xen_Number *)__instance_new(&Xen_Number_Implement, NULL);
  if (!z) { return NULL; }

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
    return NULL;
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
    return NULL;
  }

  /* Reservar arreglo de dígitos inicial: valor 0 con un limb */
  z->digits = (uint32_t *)malloc(sizeof(uint32_t));
  if (!z->digits) {
    Xen_DEL_REF(z);
    return NULL;
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
      return NULL;
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
        return NULL;
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
