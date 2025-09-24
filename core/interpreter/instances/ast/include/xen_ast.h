#ifndef __XEN_AST_H__
#define __XEN_AST_H__

#include "instance.h"
#include <stddef.h>

Xen_Instance *Xen_AST_Node_New(const char *, const char *);
int Xen_AST_Node_Push_Child(Xen_Instance *, Xen_Instance *);
const char *Xen_AST_Node_Name(Xen_Instance *);
const char *Xen_AST_Node_Value(Xen_Instance *);
Xen_Instance *Xen_AST_Node_Children(Xen_Instance *);
Xen_Instance *Xen_AST_Node_Get_Child(Xen_Instance *, size_t);

#endif
