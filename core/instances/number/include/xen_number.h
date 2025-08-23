#ifndef __XEN_NUMBER_H__
#define __XEN_NUMBER_H__

#include "xen_number_instance.h"

Xen_Number *Xen_Number_From_CString(const char *, int);

extern const signed char Xen_Char_Digit_Value[256];

#endif
