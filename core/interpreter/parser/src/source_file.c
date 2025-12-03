#include "source_file.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_typedefs.h"

Xen_Source_File* Xen_Source_File_New(Xen_c_string_t file,
                                     Xen_c_string_t content,
                                     Xen_size_t length) {
  Xen_Source_File* sf = Xen_Alloc(sizeof(Xen_Source_File));
  sf->sf_name = Xen_CString_Dup(file);
  sf->sf_content = Xen_CString_Dup(content);
  sf->sf_length = length;
  sf->sfl_offsets = NULL;
  sf->sfl_count = 0;
  sf->sfl_cap = 0;
  return sf;
}

void Xen_Source_File_Line_Push(Xen_Source_File* sf, Xen_size_t line) {
  if (sf->sfl_count >= sf->sfl_cap) {
    Xen_size_t new_cap = (sf->sfl_cap == 0) ? 4 : sf->sfl_cap * 2;
    sf->sfl_offsets =
        Xen_Realloc(sf->sfl_offsets, new_cap * sizeof(Xen_size_t));
    sf->sfl_cap = new_cap;
  }
  sf->sfl_offsets[sf->sfl_count++] = line;
}

void Xen_Source_File_Free(Xen_Source_File* sf) {
  if (!sf) {
    return;
  }
  Xen_Dealloc((void*)sf->sf_name);
  Xen_Dealloc((void*)sf->sf_content);
  if (sf->sfl_offsets) {
    Xen_Dealloc(sf->sfl_offsets);
  }
  Xen_Dealloc(sf);
}
