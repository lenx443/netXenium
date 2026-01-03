#ifndef __XEN_IGC_H__
#define __XEN_IGC_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"

typedef struct __IGC_Roots Xen_IGC_Fork;

void Xen_IGC_Init(void);
void Xen_IGC_Finish(void);

void Xen_IGC_Push(Xen_Instance*);
void Xen_IGC_Pop(void);

Xen_IGC_Fork* Xen_IGC_Fork_New(void);
void Xen_IGC_Fork_Push(Xen_IGC_Fork*, Xen_Instance*);
void Xen_IGC_Fork_Pop(Xen_IGC_Fork*);

#define Xen_IGC_XPUSH(inst, x)                                                 \
  do {                                                                         \
    Xen_IGC_Push((inst));                                                      \
    (x)++;                                                                     \
  } while (0)

#define Xen_IGC_XPOP(x)                                                        \
  do {                                                                         \
    for (Xen_size_t __i__ = 0; __i__ < (x); __i__++) {                         \
      Xen_IGC_Pop();                                                           \
    }                                                                          \
  } while (0)

static inline void Xen_IGC_Write_Field(Xen_Instance* parent,
                                       Xen_GCHandle** field,
                                       Xen_Instance* child) {
  Xen_GC_Write_Field((Xen_GCHeader*)parent, (Xen_GCHandle**)field,
                     (Xen_GCHeader*)child);
}

#define Xen_IGC_WRITE_FIELD(parent, field, child)                              \
  do {                                                                         \
    Xen_IGC_Write_Field((Xen_Instance*)(parent), (Xen_GCHandle**)&(field),     \
                        (Xen_Instance*)(child));                               \
  } while (0)

#define Xen_IGC_FORK_XPUSH(f, inst, x)                                         \
  do {                                                                         \
    Xen_IGC_Fork_Push((f), (inst));                                            \
    (x)++;                                                                     \
  } while (0)

#define Xen_IGC_FORK_XPOP(f, x)                                                \
  do {                                                                         \
    for (Xen_size_t __i__ = 0; __i__ < (x); __i__++) {                         \
      Xen_IGC_Fork_Pop((f));                                                   \
    }                                                                          \
  } while (0)

#endif
