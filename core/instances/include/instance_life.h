#ifndef __INSTANCE_LIFE_H__
#define __INSTANCE_LIFE_H__

#include "xen_igc.h"

int Xen_Instance_Init(void);
void Xen_Instance_Finish(void);

extern Xen_IGC_Fork* impls_maps;

#endif
