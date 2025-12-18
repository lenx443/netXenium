#include <stdio.h>
#include <unistd.h>

#include "netxenium/netXenium.h"

#define FILE_CAP_READ (1 << 0)
#define FILE_CAP_WRITE (1 << 1)

static Xen_Implement* File_Implement_Pointer = NULL;

typedef struct {
  Xen_INSTANCE_HEAD;
  Xen_intptr_t f;
  Xen_bool_t open;
  Xen_uint64_t caps;
} File;

static Xen_Instance* file_create(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* file_path_inst = Xen_Tuple_Get_Index(args, 0);
  if (!Xen_IsString(file_path_inst)) {
    return NULL;
  }
  Xen_c_string_t file_path = Xen_String_As_CString(file_path_inst);
  File* file = (File*)self;
  file->f = open(file_path, O_RDWR);
  if (file->f < 0) {
    return NULL;
  }
  file->open = 1;
  file->caps = FILE_CAP_READ | FILE_CAP_WRITE;
  return nil;
}

static Xen_Instance* file_destroy(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (file->open) {
    close(file->f);
  }
  return nil;
}

static Xen_Instance* file_read(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (!(file->caps & FILE_CAP_READ)) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* size_inst = Xen_Tuple_Get_Index(args, 0);
  if (!Xen_IsNumber(size_inst)) {
    return NULL;
  }
  Xen_size_t size = Xen_Number_As_Int64(size_inst);
  Xen_string_t buffer = Xen_Alloc(size + 1);
  if (read(file->f, buffer, size) < 0) {
    perror("Error");
    return NULL;
  }
  buffer[size] = '\0';
  Xen_Instance* result = Xen_String_From_CString(buffer);
  Xen_Dealloc(buffer);
  return result;
}

static Xen_Instance* file_write(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (!(file->caps & FILE_CAP_WRITE)) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value_inst = Xen_Tuple_Get_Index(args, 0);
  if (!Xen_IsString(value_inst)) {
    return NULL;
  }
  Xen_c_string_t value = Xen_String_As_CString(value_inst);
  if (write(file->f, (void*)value, Xen_SIZE(value_inst)) < 0) {
    perror("Error");
    return NULL;
  }
  return nil;
}

Xen_ImplementStruct File_Implement = {
    .__impl_name = "File",
    .__inst_size = sizeof(File),
    .__create = file_create,
    .__destroy = file_destroy,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

Xen_ImplementStruct* implements[] = {&File_Implement, Xen_NULL};

static Xen_Instance* Init(Xen_Instance* self, Xen_Instance* args,
                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if ((File_Implement_Pointer =
           (Xen_Implement*)Xen_Attr_Get_Str(self, "File")) == NULL) {
    return NULL;
  }
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "read", file_read, nil);
  Xen_VM_Store_Native_Function(props, "write", file_write, nil);
  Xen_Implement_SetProps(File_Implement_Pointer, props);
  return nil;
}

struct Xen_Module_Def* Xen_Module_io_Start(void*);
struct Xen_Module_Def* Xen_Module_io_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("io", Init, Xen_NULL, implements);
}
