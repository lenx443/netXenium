#ifndef __INSTANCE_LIFE_H__
#define __INSTANCE_LIFE_H__

#include "xen_igc.h"

int Xen_Instance_Init();
void Xen_Instance_Finish();

extern Xen_IGC_Fork* impls_maps;

#endif
