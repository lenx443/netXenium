#include <stddef.h>

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

Xen_size_t Xen_Source_File_Line_Push(Xen_Source_File* sf, Xen_size_t line) {
  if (sf->sfl_count >= sf->sfl_cap) {
    Xen_size_t new_cap = (sf->sfl_cap == 0) ? 4 : sf->sfl_cap * 2;
    sf->sfl_offsets =
        Xen_Realloc(sf->sfl_offsets, new_cap * sizeof(Xen_size_t));
    sf->sfl_cap = new_cap;
  }
  Xen_size_t sf_line = sf->sfl_count + 1;
  sf->sfl_offsets[sf->sfl_count++] = line;
  return sf_line;
}

void Xen_Source_File_Free(Xen_Source_File* sf) {
  if (!sf) {
    return;
  }
  Xen_Dealloc((void*)sf->sf_name);
  Xen_Dealloc((void*)sf->sf_content);
  Xen_Dealloc(sf->sfl_offsets);
  Xen_Dealloc(sf);
}

Xen_Source_Table* Xen_Source_Table_New(void) {
  Xen_Source_Table* st = Xen_Alloc(sizeof(Xen_Source_Table));
  st->st_files = NULL;
  st->st_count = 0;
  st->st_cap = 0;
  return st;
}

Xen_size_t Xen_Source_Table_File_Push(Xen_Source_Table* st,
                                      Xen_Source_File* sf) {
  if (st->st_count >= st->st_cap) {
    Xen_size_t new_cap = (st->st_cap == 0) ? 4 : st->st_cap * 2;
    st->st_files =
        Xen_Realloc(st->st_files, new_cap * sizeof(Xen_Source_File*));
    st->st_cap = new_cap;
  }
  Xen_size_t sf_id = st->st_count;
  st->st_files[st->st_count++] = sf;
  return sf_id;
}

void Xen_Source_Table_Free(Xen_Source_Table* st) {
  if (!st)
    return;
  for (Xen_size_t i = 0; i < st->st_count; i++) {
    Xen_Source_File_Free(st->st_files[i]);
  }
  Xen_Dealloc(st->st_files);
  Xen_Dealloc(st);
}

void Xen_Source_Table_Init(void) {
  globals_sources = Xen_Source_Table_New();
}

void Xen_Source_Table_Finish(void) {
  Xen_Source_Table_Free(globals_sources);
}

Xen_Source_Table* globals_sources = NULL;
