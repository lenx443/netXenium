#include "vm_scope.h"
#include "gc_header.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_map.h"

static void __VM_Scopes_Trace(Xen_GCHeader* h) {
  Xen_VM_Scope* scope = ((Xen_VM_Scopes*)h)->scopes;
  while (scope) {
    if (scope->symbols && scope->symbols->ptr) {
      Xen_GC_Trace_GCHeader(scope->symbols);
    }
    scope = scope->next;
  }
}

static void __VM_Scopes_Destroy(Xen_GCHeader* h) {
  Xen_VM_Scope* scope = ((Xen_VM_Scopes*)h)->scopes;
  while (scope) {
    Xen_VM_Scope* next = scope->next;
    Xen_GCHandle_Free(scope->symbols);
    Xen_Dealloc(scope);
    scope = next;
  }
  Xen_Dealloc(h);
}

Xen_VM_Scopes* Xen_VM_Scopes_New(void) {
  Xen_VM_Scopes* scopes = (Xen_VM_Scopes*)Xen_GC_New(
      sizeof(Xen_VM_Scopes), __VM_Scopes_Trace, __VM_Scopes_Destroy);
  scopes->scopes = NULL;
  return scopes;
}

void Xen_VM_Scopes_Push(Xen_VM_Scopes* scopes) {
  if (!scopes) {
    return;
  }
  Xen_VM_Scope* new_scope = (Xen_VM_Scope*)Xen_Alloc(sizeof(Xen_VM_Scope));
  Xen_GC_Push_Root((struct __GC_Header*)new_scope);
  new_scope->symbols = Xen_GCHandle_New((Xen_GCHeader*)new_scope);
  new_scope->next = NULL;
  Xen_GC_Write_Field((struct __GC_Header*)new_scope,
                     (struct __GC_Handle**)&new_scope->symbols,
                     (struct __GC_Header*)Xen_Map_New());
  if (scopes->scopes) {
    new_scope->next = scopes->scopes;
  }
  Xen_GC_Pop_Root();
  scopes->scopes = new_scope;
}

void Xen_VM_Scopes_Pop(Xen_VM_Scopes* scopes) {
  if (!scopes && !scopes->scopes) {
    return;
  }
  scopes->scopes = scopes->scopes->next;
}
