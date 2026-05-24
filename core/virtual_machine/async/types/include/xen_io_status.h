#ifndef __XEN_IO_STATUS_H__
#define __XEN_IO_STATUS_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef struct IO_Status {
  Xen_GCHeader gc;
  int closed;
#ifdef __linux
  int fd;
#endif
  Xen_GCHandle* in;
  Xen_GCHandle* out;
  Xen_uint32_t events;
} Xen_IO_Status;

Xen_IO_Status* Xen_IO_Status_New(void *);
void Xen_IO_Status_SIn(Xen_IO_Status*, Xen_Instance*);
void Xen_IO_Status_SOut(Xen_IO_Status*, Xen_Instance*);
void Xen_IO_Status_In_Wake(Xen_IO_Status*);
void Xen_IO_Status_Out_Wake(Xen_IO_Status*);
void Xen_IO_Status_Wake(Xen_IO_Status*);
void Xen_IO_Status_Close(Xen_IO_Status*);

#endif
