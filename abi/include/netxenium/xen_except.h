#ifndef __XEN_EXCEPT_H__
#define __XEN_EXCEPT_H__

#include "attrs.h"
#include "instance.h"
#include "vm.h"
#include "xen_typedefs.h"

#include <stddef.h>

Xen_Instance* Xen_Except_New(Xen_c_string_t, Xen_c_string_t);
Xen_Instance* Xen_Except_New_CFormat(Xen_c_string_t, Xen_c_string_t, ...);

#define Xen_SyntaxError(msg)                                                   \
  Xen_VM_Except_Throw(Xen_Except_New("SyntaxError", msg))

#define Xen_SyntaxError_Format(msg, ...)                                       \
  Xen_VM_Except_Throw(Xen_Except_New_CFormat("SyntaxError", msg, __VA_ARGS__))

static inline int Xen_Interrupt(void) {
  return Xen_VM_Except_Throw(
      Xen_Except_New("Interrupt", "Execution was interrupted."));
}

static inline int Xen_UndefName(Xen_c_string_t name) {
  return Xen_VM_Except_Throw(
      Xen_Except_New_CFormat("UndefError", "Name '%s' is not defined.", name));
}

static inline int Xen_UndefReg(Xen_c_string_t reg) {
  return Xen_VM_Except_Throw(Xen_Except_New_CFormat(
      "UndefError", "Register '%s' is not defined.", reg));
}

static inline int Xen_IndexError(Xen_Instance* idx) {
  Xen_c_string_t idx_str = Xen_Attr_Raw_Str(idx);
  return Xen_VM_Except_Throw(
      Xen_Except_New_CFormat("IndexError", "Cannot index with %s", idx_str));
}

static inline int Xen_IndexError_Store(Xen_Instance* idx) {
  Xen_c_string_t idx_str = Xen_Attr_Raw_Str(idx);
  return Xen_VM_Except_Throw(
      Xen_Except_New_CFormat("IndexError", "Cannot modify index %s", idx_str));
}

static inline int Xen_AttrError(Xen_c_string_t attr) {
  return Xen_VM_Except_Throw(Xen_Except_New_CFormat(
      "AttrError", "Attribute '%s' is not defined.", attr));
}

static inline int Xen_AttrError_Store(Xen_c_string_t attr) {
  return Xen_VM_Except_Throw(Xen_Except_New_CFormat(
      "AttrError", "Attribute '%s' cannot be modified.", attr));
}

static inline int Xen_OprError(void) {
  return Xen_VM_Except_Throw(
      Xen_Except_New("OprError", "Operator cannot be applied."));
}

static inline int Xen_ThrowError(void) {
  return Xen_VM_Except_Throw(
      Xen_Except_New("ThrowError", "Failed to throw exception."));
}

static inline int Xen_RangeEnd(void) {
  return Xen_VM_Except_Throw(Xen_Except_New("RangeEnd", NULL));
}

#endif
