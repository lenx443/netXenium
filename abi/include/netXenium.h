#ifndef __NETXENIUM_H__
#define __NETXENIUM_H__

#include <stdint.h>

#define XEN_INT8_SIZE 1
#define XEN_INT16_SIZE 2
#define XEN_INT32_SIZE 4
#define XEN_INT64_SIZE (8)

#define XEN_INT8_MIN -128
#define XEN_INT16_MIN -32768
#define XEN_INT32_MIN (-2147483647 - 1)
#define XEN_INT64_MIN (-9223372036854775807LL - 1)

#define XEN_INT8_MAX 127
#define XEN_INT16_MAX 32767
#define XEN_INT32_MAX 2147483647
#define XEN_INT64_MAX 9223372036854775807LL

typedef int8_t Xen_int8_t;
typedef int16_t Xen_int16_t;
typedef int32_t Xen_int32_t;
typedef int64_t Xen_int64_t;

#define XEN_UINT8_SIZE 1
#define XEN_UINT16_SIZE 2
#define XEN_UINT32_SIZE 4
#define XEN_UINT64_SIZE 8

#define XEN_UINT8_MIN 0
#define XEN_UINT16_MIN 0
#define XEN_UINT32_MIN 0
#define XEN_UINT64_MIN 0

#define XEN_UINT8_MAX 255
#define XEN_UINT16_MAX 65535
#define XEN_UINT32_MAX 4294967295
#define XEN_UINT64_MAX 18446744073709551615ULL

typedef uint8_t Xen_uint8_t;
typedef uint16_t Xen_uint16_t;
typedef uint32_t Xen_uint32_t;
typedef uint64_t Xen_uint64_t;

#define XEN_SHORT_SIZE 2
#define XEN_USHORT_SIZE 2

#define XEN_SHORT_MIN -32768
#define XEN_SHORT_MAX 32767

#define XEN_USHORT_MIN 0
#define XEN_USHORT_MAX 65535

typedef int16_t Xen_short;
typedef uint16_t Xen_ushort;

#define XEN_INT_SIZE 4
#define XEN_UINT_SIZE 4

#define XEN_INT_MIN (-2147483647 - 1)
#define XEN_INT_MAX 2147483647

#define XEN_UINT_MIN 0
#define XEN_UINT_MAX 4294967295

typedef int32_t Xen_int_t;
typedef uint32_t Xen_uint_t;

#if INTPTR_MAX == INT64_MAX
#define XEN_LONG_SIZE 8
#define XEN_ULONG_SIZE 8

#define XEN_LONG_MIN (-9223372036854775807LL - 1)
#define XEN_LONG_MAX 9223372036854775807LL

#define XEN_ULONG_MIN 0
#define XEN_ULONG_MAX 18446744073709551615ULL

typedef int64_t Xen_long_t;
typedef uint64_t Xen_ulong_t;
#else
#define XEN_LONG_SIZE 4
#define XEN_ULONG_SIZE 4

#define XEN_LONG_MIN (-2147483647 - 1)
#define XEN_LONG_MAX 2147483647

#define XEN_ULONG_MIN 0
#define XEN_ULONG_MAX 4294967295

typedef int32_t Xen_long_t;
typedef uint32_t Xen_ulong_t;
#endif

#define XEN_LLONG_SIZE 8
#define XEN_ULLONG_SIZE 8

#define XEN_LLONG_MIN (-9223372036854775807LL - 1)
#define XEN_LLONG_MAX 9223372036854775807LL

#define XEN_ULLONG_MIN 0
#define XEN_ULLONG_MAX 18446744073709551615ULL

typedef int64_t Xen_llong_t;
typedef uint64_t Xen_ullong_t;

#if INTPTR_MAX == INT64_MAX
#define XEN_SIZE_SIZE 8
#define XEN_SSIZE_SIZE 8

#define XEN_SIZE_MIN 0
#define XEN_SIZE_MAX 18446744073709551615ULL

#define XEN_SSIZE_MIN (-9223372036854775807LL - 1)
#define XEN_SSIZE_MAX 9223372036854775807LL

typedef uint64_t Xen_size_t;
typedef int64_t Xen_ssize_t;
#else
#define XEN_SIZE_SIZE 4
#define XEN_SSIZE_SIZE 4

#define XEN_SIZE_MIN 0
#define XEN_SIZE_MAX 4294967295

#define XEN_SSIZE_MIN (-2147483647 - 1)
#define XEN_SSIZE_MAX 2147483647

typedef uint32_t Xen_size_t;
typedef int32_t Xen_ssize_t;
#endif

#define XEN_INTPTR_SIZE ((Xen_size_t)sizeof(intptr_t))
#define XEN_UINTPTR_SIZE ((Xen_size_t)sizeof(uintptr_t))

#define XEN_INTPTR_MIN INTPTR_MIN
#define XEN_INTPTR_MAX INTPTR_MAX

#define XEN_UINTPTR_MIN 0
#define XEN_UINTPTR_MAX UINTPTR_MAX

typedef intptr_t Xen_intptr_t;
typedef uintptr_t Xen_uintptr_t;

#define XEN_STRING_SIZE ((Xen_size_t)sizeof(char*))
#define XEN_SSTRING_SIZE ((Xen_size_t)sizeof(signed char*))
#define XEN_USTRING_SIZE ((Xen_size_t)sizeof(unsigned char*))

typedef char* Xen_string_t;
typedef const char* Xen_c_string_t;
typedef signed char* Xen_sstring_t;
typedef const signed char* Xen_c_sstring_t;
typedef unsigned char* Xen_ustring_t;
typedef const unsigned char* Xen_c_ustring_t;

#define XEN_BOOL_SIZE 1

#define XEN_BOOL_MIN 0
#define XEN_BOOL_MAX 1

#define XEN_BOOL(v) ((v) ? 1 : 0)

typedef uint8_t Xen_bool_t;

struct Xen_Module_Def;
typedef struct __Instance Xen_Instance;

struct Xen_Module_Def* Xen_Module_Define(Xen_c_string_t);
void Xen_Debug_Print(Xen_c_string_t, ...);

#endif
