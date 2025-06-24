#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "eval.h"

char *eval_arg(const ArgExpr_t *arg) {
  if (!arg) return NULL;
  switch (arg->arg_type) {
  case ARG_LITERAL: return strdup(arg->literal);
  case ARG_PROPERTY:
    // Aquí podrías buscar el valor de la propiedad, por ahora solo lo devuelve
    return strdup(arg->property);
  case ARG_CONCAT: {
    // Concatenar todos los resultados de las partes
    size_t total_len = 1; // 1 para el '\0'
    char **part_strs = malloc(sizeof(char *) * arg->concat.count);
    for (int i = 0; i < arg->concat.count; ++i) {
      part_strs[i] = eval_arg(arg->concat.parts[i]);
      if (part_strs[i]) total_len += strlen(part_strs[i]);
    }
    char *result = malloc(total_len);
    result[0] = '\0';
    for (int i = 0; i < arg->concat.count; ++i) {
      if (part_strs[i]) strcat(result, part_strs[i]);
      free(part_strs[i]);
    }
    free(part_strs);
    return result;
  }
  default: return NULL;
  }
}

// Evalúa el AST de un comando e imprime el comando y sus argumentos evaluados
void ast_eval(const AST_Node_t *node) {
  if (!node) return;
  puts("Evaluing...");
  switch (node->ast_type) {
  case AST_CMD:
    printf("Comando: %s\n", node->cmd.cmd_name);
    for (int i = 0; i < node->cmd.arg_count; ++i) {
      char *arg_val = eval_arg(node->cmd.cmd_args[i]);
      printf("  Arg %d: %s\n", i + 1, arg_val ? arg_val : "(null)");
      free(arg_val);
    }
    break;
  default: printf("Tipo de nodo AST no soportado.\n");
  }
}
