#ifndef __SOURCE_FILE_H__
#define __SOURCE_FILE_H__

#include "xen_typedefs.h"

struct __Source_File {
  Xen_c_string_t sf_name;
  Xen_c_string_t sf_content;
  Xen_size_t sf_length;
  Xen_size_t* sfl_offsets;
  Xen_size_t sfl_count;
  Xen_size_t sfl_cap;
};

struct __Source_Table {
  struct __Source_File** st_files;
  Xen_size_t st_count;
  Xen_size_t st_cap;
};

struct __Source_Address {
  Xen_size_t id;
  Xen_size_t line;
  Xen_size_t column;
};

typedef struct __Source_File Xen_Source_File;
typedef struct __Source_Table Xen_Source_Table;
typedef struct __Source_Address Xen_Source_Address;

Xen_Source_File* Xen_Source_File_New(Xen_c_string_t, Xen_c_string_t,
                                     Xen_size_t);
void Xen_Source_File_Line_Push(Xen_Source_File*, Xen_size_t);
void Xen_Source_File_Free(Xen_Source_File*);

Xen_Source_Table* Xen_Source_Table_New(void);
Xen_size_t Xen_Source_Table_File_Push(Xen_Source_Table*, Xen_Source_File*);
void Xen_Source_Table_Free(Xen_Source_Table*);

void Xen_Source_Table_Init(void);
void Xen_Source_Table_Finish(void);

extern Xen_Source_Table* globals_sources;

#endif
