#ifndef __BC_INSTRUCT_H__
#define __BC_INSTRUCT_H__

#include "source_file.h"
#include "xen_typedefs.h"

#pragma pack(push, 1)
struct bc_Instruct_Header {
  Xen_uint8_t bci_opcode;
  Xen_uint8_t bci_oparg;
};
#pragma pack(pop)

struct bc_Instruct {
  struct bc_Instruct_Header hdr;
  Xen_Source_Address sta;
};

typedef struct bc_Instruct bc_Instruct_t;
typedef bc_Instruct_t* bc_Instruct_ptr;

#endif
