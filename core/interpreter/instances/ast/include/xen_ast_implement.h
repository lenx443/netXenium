#ifndef __XEN_AST_IMPLEMENT_H__
#define __XEN_AST_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_AST_GetImplement(void);

int Xen_AST_Init(void);
void Xen_AST_Finish(void);

#endif
