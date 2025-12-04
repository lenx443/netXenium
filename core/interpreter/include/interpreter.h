#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <stdint.h>

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* interpreter(Xen_c_string_t, const char*, uint8_t);

#endif
