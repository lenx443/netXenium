#ifndef __XEN_NUMBER_H__
#define __XEN_NUMBER_H__

#include "xen_number_instance.h"

Xen_Number *Xen_Number_From_CString(const char *, int);
Xen_Number *Xen_Number_From_Int32(int32_t);
Xen_Number *Xen_Number_From_Int64(int64_t);
Xen_Number *Xen_Number_From_Int(int);
Xen_Number *Xen_Number_From_UInt(unsigned int);
Xen_Number *Xen_Number_From_Long(long);
Xen_Number *Xen_Number_From_ULong(unsigned long);
Xen_Number *Xen_Number_From_LongLong(long long);
Xen_Number *Xen_Number_From_ULongLong(unsigned long long);

extern const signed char Xen_Char_Digit_Value[256];

#endif
