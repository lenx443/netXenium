#include "ast.h"
#include "lex.yy.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
extern int yyparse(void);
extern YY_BUFFER_STATE yy_scan_string(const char *);
extern void yy_delete_buffer(YY_BUFFER_STATE buf);
AST_List_t *BISON_OUT;

int main() {
  const char *code = "x = \"Hola\""; // o "x = \"Hola\"\ny = \"Mundo\""
  YY_BUFFER_STATE buf = yy_scan_string(code);
  if (yyparse() == 0) { printf("Parsing correcto!\n"); }
  yy_delete_buffer(buf);
  return 0;
}
