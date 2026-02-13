#ifndef __VM_SCOPE_H__
#define __VM_SCOPE_H__

typedef struct __VM_Scope Xen_VM_Scope;
typedef struct __VM_Scopes Xen_VM_Scopes;

Xen_VM_Scopes* Xen_VM_Scopes_New(void);
void Xen_VM_Scopes_Push(Xen_VM_Scopes*);
void Xen_VM_Scopes_Pop(Xen_VM_Scopes*);

#endif
