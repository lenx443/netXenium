#include <asm-generic/fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "netxenium/netXenium.h"
#include "netxenium/xen_function.h"

#define FILE_CAP_READ (1 << 0)
#define FILE_CAP_WRITE (1 << 1)
#define FILE_CAP_SEEK (1 << 2)

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
  Xen_Function_ArgSpec args_def[] = {
      {"path", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       1, NULL},
      {"read", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_BOOLEAN, 0,
       Xen_True},
      {"write", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_BOOLEAN, 0,
       Xen_False},
      {"create", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_BOOLEAN,
       0, Xen_False},
      {"append", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_BOOLEAN,
       0, Xen_False},
      {"trunc", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_BOOLEAN, 0,
       Xen_False},
      {"mode", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_NUMBER, 0,
       nil},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  Xen_Instance* file_path_inst =
      Xen_Function_ArgBinding_Search(args_binding, "path")->value;
  Xen_Instance* cap_read =
      Xen_Function_ArgBinding_Search(args_binding, "read")->value;
  Xen_Instance* cap_write =
      Xen_Function_ArgBinding_Search(args_binding, "write")->value;
  Xen_Instance* cap_create =
      Xen_Function_ArgBinding_Search(args_binding, "create")->value;
  Xen_Instance* cap_append =
      Xen_Function_ArgBinding_Search(args_binding, "append")->value;
  Xen_Instance* cap_trunc =
      Xen_Function_ArgBinding_Search(args_binding, "trunc")->value;
  Xen_uint32_t mode = 0666;
  if (Xen_Function_ArgBinding_Search(args_binding, "mode")->provided) {
    mode = Xen_Number_As_Int32(
        Xen_Function_ArgBinding_Search(args_binding, "mode")->value);
  }
  Xen_Function_ArgBinding_Free(args_binding);
  File* file = (File*)self;
  Xen_uint32_t flags = 0;
  if (cap_read == Xen_True && cap_write == Xen_True) {
    file->caps |= FILE_CAP_READ;
    file->caps |= FILE_CAP_WRITE;
    flags |= O_RDWR;
  } else if (cap_read == Xen_True) {
    file->caps |= FILE_CAP_READ;
    flags |= O_RDONLY;
  } else if (cap_write == Xen_True) {
    file->caps |= FILE_CAP_WRITE;
    flags |= O_WRONLY;
  }
  if (cap_create == Xen_True) {
    flags |= O_CREAT;
  }
  if (cap_append == Xen_True) {
    flags |= O_APPEND;
  }
  if (cap_trunc == Xen_True) {
    flags |= O_TRUNC;
  }
  Xen_c_string_t file_path = Xen_String_As_CString(file_path_inst);
  if (cap_create == Xen_True) {
    file->f = open(file_path, flags, mode);
  } else {
    file->f = open(file_path, flags);
  }
  if (file->f < 0) {
    return NULL;
  }
  file->open = 1;
  if (lseek(file->f, 0, SEEK_CUR) >= 0) {
    file->caps |= FILE_CAP_SEEK;
  }
  return nil;
}

static Xen_Instance* file_destroy(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (file->open) {
    close(file->f);
    file->open = 0;
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
  Xen_Function_ArgSpec args_def[] = {
      {"size", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       0, nil},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  if (Xen_Function_ArgBinding_Search(args_binding, "size")->provided) {
    Xen_Instance* size_inst =
        Xen_Function_ArgBinding_Search(args_binding, "size")->value;
    Xen_Function_ArgBinding_Free(args_binding);
    Xen_size_t size = Xen_Number_As_Int64(size_inst);
    Xen_uint8_t* buffer = Xen_Alloc(size);
    Xen_ssize_t r = read(file->f, buffer, size);
    if (r < 0) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* result = Xen_Bytes_From_Array(r, buffer);
    Xen_Dealloc(buffer);
    return result;
  } else {
    Xen_Function_ArgBinding_Free(args_binding);
    const Xen_size_t chunk = 4096;
    Xen_size_t cap = chunk;
    Xen_size_t len = 0;
    Xen_uint8_t* buffer = Xen_Alloc(cap);
    for (;;) {
      if (len + chunk > cap) {
        cap *= 2;
        buffer = Xen_Realloc(buffer, cap);
      }
      ssize_t r = read(file->f, buffer + len, chunk);
      if (r < 0) {
        Xen_Dealloc(buffer);
        return NULL;
      }
      if (r == 0) {
        break;
      }
      len += r;
    }
    Xen_Instance* result = Xen_Bytes_From_Array(len, buffer);
    Xen_Dealloc(buffer);
    return result;
  }
  return NULL;
}

static Xen_Instance* file_write(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (!(file->caps & FILE_CAP_WRITE)) {
    return NULL;
  }
  static Xen_Function_ArgSpec args_def[] = {
      {"data", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES, 1,
       NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  Xen_Instance* data =
      Xen_Function_ArgBinding_Search(args_binding, "data")->value;
  Xen_Function_ArgBinding_Free(args_binding);
  Xen_ssize_t w = write(file->f, Xen_Bytes_Get(data), Xen_SIZE(data));
  if (w < 0) {
    return NULL;
  }
  return Xen_Number_From_Long(w);
}

static Xen_Instance* file_seek(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  File* file = (File*)self;
  if (!(file->caps & FILE_CAP_SEEK)) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"offset", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       1, NULL},
      {"whence", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       0, nil},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  Xen_Instance* offset_inst =
      Xen_Function_ArgBinding_Search(args_binding, "offset")->value;
  Xen_size_t offset = Xen_Number_As_Int64(offset_inst);
  Xen_int_t whence = SEEK_CUR;
  if (Xen_Function_ArgBinding_Search(args_binding, "whence")->provided) {
    Xen_Instance* whence_inst =
        Xen_Function_ArgBinding_Search(args_binding, "whence")->value;
    if (whence_inst) {
      int whence_val = Xen_Number_As_Int(whence_inst);
      if (whence_val == 0) {
        whence = SEEK_SET;
      } else if (whence_val == 1) {
        whence = SEEK_CUR;
      } else if (whence_val == 2) {
        whence = SEEK_END;
      } else {
        return NULL;
      }
    }
  }
  off_t pos = lseek(file->f, offset, whence);
  if (pos < 0) {
    return NULL;
  }
  return Xen_Number_From_Long(pos);
}

static Xen_Instance* file_close(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!Xen_Function_ArgEmpy(args, kwargs)) {
    return NULL;
  }
  File* file = (File*)self;
  if (file->open) {
    close(file->f);
    file->open = 0;
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
  Xen_VM_Store_Native_Function(props, "seek", file_seek, nil);
  Xen_VM_Store_Native_Function(props, "close", file_close, nil);
  Xen_Map_Push_Pair_Str(props,
                        (Xen_Map_Pair_Str){"SEEK_SET", Xen_Number_From_Int(0)});
  Xen_Map_Push_Pair_Str(props,
                        (Xen_Map_Pair_Str){"SEEK_CUR", Xen_Number_From_Int(1)});
  Xen_Map_Push_Pair_Str(props,
                        (Xen_Map_Pair_Str){"SEEK_END", Xen_Number_From_Int(2)});
  Xen_Implement_SetProps(File_Implement_Pointer, props);
  return nil;
}

struct Xen_Module_Def* Xen_Module_io_Start(void*);
struct Xen_Module_Def* Xen_Module_io_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("io", Init, Xen_NULL, implements);
}
