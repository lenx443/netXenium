#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "commands.h"
#include "eval.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "properties.h"

char *eval_arg(const ArgExpr_t *arg) {
  if (!arg) return NULL;
  switch (arg->arg_type) {
  case ARG_LITERAL: return strdup(arg->literal);
  case ARG_PROPERTY: {
    prop_struct *prop_s = prop_reg_value(arg->property, *prop_register);
    if (!prop_s) {
      log_add(NULL, ERROR, program.name, "No se encontro la propiedad %s", arg->property);
      return NULL;
    }
    return strdup(prop_s->value);
  }
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

void ast_eval(const AST_Node_t *node) {
  if (!node) return;
  switch (node->ast_type) {
  case AST_EMPTY: break;
  case AST_CMD: {
    LIST_ptr args = list_new();
    list_push_back_string_node(args, node->cmd.cmd_name);
    for (int i = 0; i < node->cmd.arg_count; ++i) {
      char *arg_val = eval_arg(node->cmd.cmd_args[i]);
      if (!arg_val) {
        log_add(NULL, ERROR, program.name, "Error al formar argumentos");
        log_show_and_clear(NULL);
        program.return_code = EXIT_FAILURE;
        list_free(args);
        return;
      }
      list_push_back_string_node(args, arg_val);
      free(arg_val);
    }
    for (int i = 0; cmds_table[i] != NULL; i++) {
      if (strcmp(cmds_table[i]->name, node->cmd.cmd_name) == 0) {
        int args_size = list_size(*args);
        if ((args_size - 1) < cmds_table[i]->args_len[0] ||
            (args_size - 1) > cmds_table[i]->args_len[1])
          break;
        program.return_code = cmds_table[i]->func(args);
        break;
      }
    }
    free(args);
    break;
  }
  default: printf("Tipo de nodo AST no soportado.\n");
  }
}
