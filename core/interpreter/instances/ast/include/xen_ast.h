#ifndef __XEN_AST_H__
#define __XEN_AST_H__

#include <stddef.h>

#include "instance.h"

Xen_Instance* Xen_AST_Node_New(const char*, const char*);
int Xen_AST_Node_Push_Child(Xen_Instance*, Xen_Instance*);
const char* Xen_AST_Node_Name(Xen_Instance*);
const char* Xen_AST_Node_Value(Xen_Instance*);
int Xen_AST_Node_Name_Cmp(Xen_Instance*, const char*);
int Xen_AST_Node_Value_Cmp(Xen_Instance*, const char*);
size_t Xen_AST_Node_Children_Size(Xen_Instance*);
Xen_Instance* Xen_AST_Node_Children(Xen_Instance*);
Xen_Instance* Xen_AST_Node_Get_Child(Xen_Instance*, size_t);
Xen_Instance* Xen_AST_Node_Wrap(Xen_Instance*, const char*);

#ifndef NDEBUG
void Xen_AST_Node_Print(Xen_Instance*);
#endif

#endif
