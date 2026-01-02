#ifndef __XEN_GC_H__
#define __XEN_GC_H__

#include "gc_header.h"
#include "xen_typedefs.h"
#include <assert.h>
#include <stdint.h>

void Xen_GC_GetReady(void);
struct __GC_Header* Xen_GC_New(Xen_size_t, void (*)(struct __GC_Header*),
                               void (*)(struct __GC_Header*));
void Xen_GC_MinorCollect(void);
void Xen_GC_MajorCollect(void);
void Xen_GC_Push_Root(struct __GC_Header*);
void Xen_GC_Pop_Root(void);
void Xen_GC_Pop_Roots(Xen_size_t);
void Xen_GC_Push_Gray(struct __GC_Header*);
struct __GC_Header* Xen_GC_Pop_Gray(void);
void Xen_GC_Trace_GCHeader(struct __GC_Header*);
void Xen_GC_Remove_RS_Child(struct __GC_Header*);
void Xen_GC_Promote_toOld(struct __GC_Header*);
void Xen_GC_Trace(struct __GC_Header*);
void Xen_GC_Reset_Young(void);
void Xen_GC_Reset_Old(void);
void Xen_GC_Reset_All(void);
void Xen_GC_Mark(void);
void Xen_GC_Mark_Young(void);
void Xen_GC_Sweep_Young(void);
void Xen_GC_Sweep_Young_Major(void);
void Xen_GC_Sweep_Old(void);
void Xen_GC_Shutdown(void);
void Xen_GC_Start(void);
void Xen_GC_Stop(void);

void Xen_GC_Write_Field(struct __GC_Header*, struct __GC_Header**,
                        struct __GC_Header*);

#endif
