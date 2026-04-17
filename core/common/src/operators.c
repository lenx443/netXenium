#include "operators.h"
#include "attrs.h"
#include "instance.h"
#include "xen_igc.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

struct Xen_Operator_Info Xen_Operators_Map[Xen_OPR_END]  = {
    [Xen_OPR_POW]         = {"__pow",         "**",  0},             [Xen_OPR_MUL]         = {"__mul",         "*",   0},
    [Xen_OPR_DIV]         = {"__div",         "/",   0},             [Xen_OPR_MOD]         = {"__mod",         "%",   0},
    [Xen_OPR_ADD]         = {"__add",         "+",   0},             [Xen_OPR_SUB]         = {"__sub",         "-",   0},
    [Xen_OPR_LT]          = {"__lt",          "<",   0},             [Xen_OPR_LE]          = {"__le",          "<=",  0},
    [Xen_OPR_EQ]          = {"__eq",          "==",  0},             [Xen_OPR_GT]          = {"__gt",          ">",   0},
    [Xen_OPR_GE]          = {"__ge",          ">=",  0},             [Xen_OPR_NE]          = {"__ne",          "!=",  0},
    [Xen_OPR_HAS]         = {"__has",         "has", 0},             [Xen_OPR_BAND]        = {"__band",        "&",   0},
    [Xen_OPR_BXOR]        = {"__bxor",        "^",   0},             [Xen_OPR_BOR]         = {"__bor",         "|",   0},
    [Xen_OPR_SHL]         = {"__shl",         "<<",  0},             [Xen_OPR_SHR]         = {"__shr",         ">>",  0},
    [Xen_OPR_ASSIGN_POW]  = {"__assign_pow",  "**=", Xen_OPR_POW},   [Xen_OPR_ASSIGN_MUL]  = {"__assign_mul",  "*=",  Xen_OPR_MUL},
    [Xen_OPR_ASSIGN_DIV]  = {"__assign_div",  "/=",  Xen_OPR_DIV},   [Xen_OPR_ASSIGN_MOD]  = {"__assign_mod",  "%=",  Xen_OPR_MOD},
    [Xen_OPR_ASSIGN_ADD]  = {"__assign_add",  "+=",  Xen_OPR_ADD},   [Xen_OPR_ASSIGN_SUB]  = {"__assign_sub",  "-=",  Xen_OPR_SUB},
    [Xen_OPR_ASSIGN_BAND] = {"__assign_band", "&=",  Xen_OPR_BAND},  [Xen_OPR_ASSIGN_BXOR] = {"__assign_bxor", "^=",  Xen_OPR_BXOR},
    [Xen_OPR_ASSIGN_BOR]  = {"__assign_bor",  "|=",  Xen_OPR_BOR},   [Xen_OPR_ASSIGN_SHL]  = {"__assign_shl",  "<<=", Xen_OPR_SHL},
    [Xen_OPR_ASSIGN_SHR]  = {"__assign_shr",  ">>=", Xen_OPR_SHR},
};

static Xen_Instance* get_operator_method(Xen_Instance* inst, Xen_Opr op) {
  Xen_Instance* method = NULL;
  Xen_Opr current_op = op;
  while (method == NULL && current_op != 0) {
    method = Xen_Attr_Get_Str(inst, Xen_Operators_Map[current_op].keyword);
    current_op = Xen_Operators_Map[current_op].dfault;
  }
  return method;
}

Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance* first, Xen_Instance* second, Xen_Opr op) {
  if (op >= Xen_OPR_END) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Instance* method = get_operator_method(first, op);
  if (!method) {
    return NULL;
  }
  Xen_IGC_XPUSH(method, roots);
  Xen_Instance* args = Xen_Tuple_From_Array(1, &second);
  if (!args) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPUSH(args, roots);
  Xen_Instance* result = Xen_Method_Call(method, args, nil);
  if (!result) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPOP(roots);
  return result;
}
