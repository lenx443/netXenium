#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "gc_header.h"

typedef uint8_t Xen_Instance_Flag;

#define _WITH_JOIN(a, b) a##b
#define _WITH_UNIQUE(a, b) _WITH_JOIN(a, b)

#define WITH_INSTANCE(var, expr)                                               \
  for (struct {                                                                \
         int active;                                                           \
         Xen_Instance* val;                                                    \
       } _WITH_UNIQUE(_ctx_, __LINE__) = {1, (expr)};                          \
       _WITH_UNIQUE(_ctx_, __LINE__).active;                                   \
       (Xen_DEL_REF(_WITH_UNIQUE(_ctx_, __LINE__).val),                        \
        _WITH_UNIQUE(_ctx_, __LINE__).active = 0))                             \
    for (Xen_Instance* var = _WITH_UNIQUE(_ctx_, __LINE__).val;                \
         _WITH_UNIQUE(_ctx_, __LINE__).active;                                 \
         _WITH_UNIQUE(_ctx_, __LINE__).active = 0)

#define Xen_INSTANCE_HEAD                                                      \
  struct __GC_Header __gc;                                                     \
  size_t __refers;                                                             \
  struct __Implement* __impl;                                                  \
  size_t __size;                                                               \
  Xen_Instance_Flag __flags;

#define Xen_INSTANCE_MAPPED_HEAD                                               \
  Xen_INSTANCE_HEAD;                                                           \
  struct __Instance* __map;

#define Xen_INSTANCE struct __Instance

#define Xen_INSTANCE_MAPPED struct __Instance_Mapped

#define Xen_INSTANCE_SET(refers, impl, flags)                                  \
  .__refers = refers, .__impl = impl, .__flags = (flags)

#define XEN_INSTANCE_FLAG_STATIC (1 << 0)
#define XEN_INSTANCE_FLAG_MAPPED (1 << 1)

#define XEN_INSTANCE_GET_FLAG(inst, flag)                                      \
  (((((struct __Instance*)inst)->__flags) & (flag)) != 0)

Xen_INSTANCE{Xen_INSTANCE_HEAD};
Xen_INSTANCE_MAPPED{Xen_INSTANCE_MAPPED_HEAD};

Xen_INSTANCE* __instance_new(struct __Implement*, Xen_INSTANCE*, Xen_INSTANCE*,
                             Xen_Instance_Flag);
void __instance_free(Xen_INSTANCE*);

typedef Xen_INSTANCE Xen_Instance;
typedef Xen_INSTANCE_MAPPED Xen_Instance_Mapped;

static inline void __DEL_REF(void* inst) {
  if (inst && ((struct __Instance*)inst)->__refers > 0) {
    if (--(((struct __Instance*)inst)->__refers) == 0)
      __instance_free((struct __Instance*)(inst));
  }
}

#ifndef XEN_DEBUG_REFERS
#define Xen_ADD_REF(inst)                                                      \
  ((inst) ? (((struct __Instance*)(inst))->__refers++, (inst)) : (inst))
#define Xen_DEL_REF(inst) __DEL_REF((void*)inst)
#else
#define Xen_ADD_REF(inst)                                                      \
  ((inst)                                                                      \
       ? (((struct __Instance*)(inst))->__refers++,                            \
          printf("[ADD_REF] %s=%p ref=%zd at %s:%d\n", #inst, inst,            \
                 ((struct __Instance*)(inst))->__refers, __FILE__, __LINE__),  \
          (inst))                                                              \
       : (inst))

#define Xen_DEL_REF(inst)                                                      \
  do {                                                                         \
    if (inst) {                                                                \
      printf("[DEL_REF] %s=%p ref=%zd at %s:%d\n", #inst, (Xen_Instance*)inst, \
             ((struct __Instance*)(inst))->__refers, __FILE__, __LINE__);      \
      ((struct __Instance*)(inst))->__refers--;                                \
      if (((struct __Instance*)(inst))->__refers <= 0)                         \
        __instance_free((struct __Instance*)(inst));                           \
    }                                                                          \
  } while (0)
#endif

static inline struct __Implement* Xen_IMPL(void* inst) {
  return (((struct __Instance*)inst)->__impl);
}

static inline size_t Xen_SIZE(void* inst) {
  return (((struct __Instance*)inst)->__size);
}

#endif
