#ifndef __XEN_NUMBER_H__
#define __XEN_NUMBER_H__

#include "instance.h"

Xen_bool_t Xen_IsNumber(Xen_Instance*);

Xen_Instance* Xen_Number_Trunc(Xen_Instance*);
Xen_Instance* Xen_Number_Floor(Xen_Instance*);
int Xen_Number_Is_Zero(Xen_Instance*);
int Xen_Number_Is_Odd(Xen_Instance*);
int Xen_Number_Cmp(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Copy(Xen_Instance*);
Xen_Instance* Xen_Number_Div2(Xen_Instance*);

Xen_INSTANCE* Xen_Number_From_CString(const char*, int);
Xen_INSTANCE* Xen_Number_From_Int32(int32_t);
Xen_INSTANCE* Xen_Number_From_Int64(int64_t);
Xen_INSTANCE* Xen_Number_From_Int(int);
Xen_INSTANCE* Xen_Number_From_UInt(unsigned int);
Xen_INSTANCE* Xen_Number_From_Long(long);
Xen_INSTANCE* Xen_Number_From_ULong(unsigned long);
Xen_INSTANCE* Xen_Number_From_LongLong(long long);
Xen_INSTANCE* Xen_Number_From_ULongLong(unsigned long long);
Xen_INSTANCE* Xen_Number_From_Pointer(void*);

const char* Xen_Number_As_CString(Xen_INSTANCE*);
int32_t Xen_Number_As_Int32(Xen_INSTANCE*);
int64_t Xen_Number_As_Int64(Xen_INSTANCE*);
int Xen_Number_As_Int(Xen_INSTANCE*);
unsigned int Xen_Number_As_UInt(Xen_INSTANCE*);
long Xen_Number_As_Long(Xen_INSTANCE*);
unsigned long Xen_Number_As_ULong(Xen_INSTANCE*);
long long Xen_Number_As_LongLong(Xen_INSTANCE*);
unsigned long long Xen_Number_As_ULongLong(Xen_INSTANCE*);
Xen_uint8_t* Xen_Number_As_Bytes(Xen_Instance*, Xen_size_t*);
Xen_uint8_t* Xen_Number_As_Bytes_Flexible(Xen_Instance*, Xen_size_t*,
                                          Xen_size_t, int, Xen_uint8_t, int);

Xen_Instance* Xen_Number_Mul(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Div(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Mod(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Pow(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Add(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Number_Sub(Xen_Instance*, Xen_Instance*);

#define Xen_Number_As(T, inst)                                                 \
  _Generic((T)0,                                                               \
      const char*: Xen_Number_As_CString,                                      \
      int32_t: Xen_Number_As_Int32,                                            \
      int64_t: Xen_Number_As_Int64,                                            \
      unsigned int: Xen_Number_As_UInt,                                        \
      unsigned long: Xen_Number_As_ULong,                                      \
      long long: Xen_Number_As_LongLong,                                       \
      unsigned long long: Xen_Number_As_ULongLong)(inst)

extern const signed char Xen_Char_Digit_Value[256];

#endif
