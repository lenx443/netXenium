#ifndef __XEN_IGC_H__
#define __XEN_IGC_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"

void Xen_IGC_Init();
void Xen_IGC_Finish();

void Xen_IGC_Push(Xen_Instance*);
void Xen_IGC_Pop();

#define Xen_IGC_XPUSH(inst, x)                                                 \
  do {                                                                         \
    Xen_IGC_Push((inst));                                                      \
    (x)++;                                                                     \
  } while (0)

#define Xen_IGC_XPOP(x)                                                        \
  do {                                                                         \
    for (Xen_size_t i = 0; i < (x); i++) {                                     \
      Xen_IGC_Pop();                                                           \
    }                                                                          \
  } while (0)

static inline void Xen_IGC_Write_Field(Xen_Instance* parent,
                                       Xen_Instance** field,
                                       Xen_Instance* child) {
  Xen_GC_Write_Field((Xen_GCHeader*)parent, (Xen_GCHeader**)field,
                     (Xen_GCHeader*)child);
}

#define Xen_IGC_WRITE_FIELD(parent, field, child)                              \
  do {                                                                         \
    Xen_IGC_Write_Field((Xen_Instance*)(parent), (Xen_Instance**)&(field),     \
                        (Xen_Instance*)(child));                               \
  } while (0)

#endif
