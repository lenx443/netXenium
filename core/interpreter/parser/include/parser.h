#ifndef __PARSER_H__
#define __PARSER_H__

#include "instance.h"
#include "lexer.h"
#include "xen_typedefs.h"

typedef struct {
  Lexer* lexer;
  Lexer_Token token;
} Parser;

Xen_Instance* Xen_Parser(Xen_c_string_t, Xen_c_string_t, Xen_size_t);

#endif
