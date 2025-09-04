#ifndef __XEN_NUMBER_H__
#define __XEN_NUMBER_H__

#include "instance.h"

Xen_INSTANCE *Xen_Number_From_CString(const char *, int);
Xen_INSTANCE *Xen_Number_From_Int32(int32_t);
Xen_INSTANCE *Xen_Number_From_Int64(int64_t);
Xen_INSTANCE *Xen_Number_From_Int(int);
Xen_INSTANCE *Xen_Number_From_UInt(unsigned int);
Xen_INSTANCE *Xen_Number_From_Long(long);
Xen_INSTANCE *Xen_Number_From_ULong(unsigned long);
Xen_INSTANCE *Xen_Number_From_LongLong(long long);
Xen_INSTANCE *Xen_Number_From_ULongLong(unsigned long long);

const char *Xen_Number_As_CString(Xen_INSTANCE *);
int32_t Xen_Number_As_Int32(Xen_INSTANCE *);
int64_t Xen_Number_As_Int64(Xen_INSTANCE *);
int Xen_Number_As_Int(Xen_INSTANCE *);
unsigned int Xen_Number_As_UInt(Xen_INSTANCE *);
long Xen_Number_As_Long(Xen_INSTANCE *);

extern const signed char Xen_Char_Digit_Value[256];

#endif
