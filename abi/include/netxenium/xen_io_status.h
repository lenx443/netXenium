#ifndef __XEN_IO_STATUS_H__
#define __XEN_IO_STATUS_H__

#include "instance.h"

typedef struct IO_Status Xen_IO_Status;

Xen_IO_Status* Xen_IO_Status_New(void *);
void* Xen_IO_Status_FD(Xen_IO_Status*);
void Xen_IO_Status_SIn(Xen_IO_Status*, Xen_Instance*);
void Xen_IO_Status_SOut(Xen_IO_Status*, Xen_Instance*);
void Xen_IO_Status_In_Wake(Xen_IO_Status*);
void Xen_IO_Status_Out_Wake(Xen_IO_Status*);
void Xen_IO_Status_Wake(Xen_IO_Status*);
void Xen_IO_Status_Close(Xen_IO_Status*);

#endif
