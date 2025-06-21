#include <stdlib.h>
#include <string.h>

#include "interpreter.h"
#include "list.h"
#include "logs.h"
#include "parser.h"
#include "program.h"
#include "properties.h"
#include "raw_arguments.h"

static int argument_inter(NODE_ptr *, char **);
static int property_inter(NODE_ptr *, char **);
static int concat_inter(NODE_ptr *, char *, char **);

void interpreter(Parser_Code *code, const char *line) {
  if (code->cdg_type == CDG_NULL)
    return;
  else if (code->cdg_type == CDG_COMMAND) {
    LIST_ptr args = list_new();
    if (args == NULL) {
      log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
      log_show_and_clear(NULL);
      return;
    }
    Raw_Arguments ras = code->cdg_value.command.args;
    NODE_ptr node = NULL;
    FOR_EACH(&node, *ras) {
      Raw_Argument *ra = (Raw_Argument *)node->point;
      if (ra->ra_type == RAT_ARGUMENT) {
        char *result = NULL;
        int n = argument_inter(&node, &result);
        if (n == 0) {
          if (!list_push_back_string_node(args, result)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            list_free(args);
            free(result);
            return;
          }
          free(result);
        } else if (n == 1) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          list_free(args);
          return;
        }
      } else if (ra->ra_type == RAT_PROPERTY) {
        char *result = NULL;
        int n = property_inter(&node, &result);
        if (n == 0) {
          if (!list_push_back_string_node(args, result)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            list_free(args);
            free(result);
            return;
          }
          free(result);
        } else if (n == 1) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          list_free(args);
          return;
        }
      }
    }
    int args_size = list_size(*args);
    char *cmd_name = code->cdg_value.command.command->name;
    if ((args_size - 1) < code->cdg_value.command.command->args_len[0] ||
        (args_size - 1) > code->cdg_value.command.command->args_len[1]) {
      if (code->cdg_value.command.command->args_len[0] ==
          code->cdg_value.command.command->args_len[1])
        log_add(NULL, ERROR, program.name, "El commando '%s' requiere %d argumentos",
                cmd_name, code->cdg_value.command.command->args_len[0]);
      else
        log_add(NULL, ERROR, program.name,
                "El commando '%s' requiere de %d-%d argumentos", cmd_name,
                code->cdg_value.command.command->args_len[0],
                code->cdg_value.command.command->args_len[1]);
      log_add(NULL, ERROR, program.name, "Numero de argumentos pasados: %d",
              args_size - 1);
      log_show_and_clear(NULL);
      free(args);
      program.return_code = 153;
      return;
    }
    program.return_code = code->cdg_value.command.command->func(args);
    if (program.return_code == 153) {
      log_add(NULL, ERROR, program.name, "hay un error sintactico en '%s'", line);
      log_show_and_clear(NULL);
      list_free(args);
      return;
    }
    list_free(args);
  } else if (code->cdg_type == CDG_ASIGN_PROP) {
    LIST_ptr args = list_new();
    if (args == NULL) {
      log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
      log_show_and_clear(NULL);
      return;
    }
    Raw_Arguments ras = code->cdg_value.assign_prop.new_value;
    NODE_ptr node = NULL;
    FOR_EACH(&node, *ras) {
      Raw_Argument *ra = (Raw_Argument *)node->point;
      if (ra->ra_type == RAT_ARGUMENT) {
        char *result = NULL;
        int n = argument_inter(&node, &result);
        if (n == 0) {
          if (!list_push_back_string_node(args, result)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            list_free(args);
            free(result);
            return;
          }
          free(result);
        } else if (n == 1) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          list_free(args);
          return;
        }
      } else if (ra->ra_type == RAT_PROPERTY) {
        char *result = NULL;
        int n = property_inter(&node, &result);
        if (n == 0) {
          if (!list_push_back_string_node(args, result)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            list_free(args);
            free(result);
            return;
          }
          free(result);
        } else if (n == 1) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          list_free(args);
          return;
        }
      } else {
        log_add(NULL, ERROR, program.name, "El valor pasado es invalido");
        log_show_and_clear(NULL);
        free(args);
        program.return_code = EXIT_FAILURE;
        return;
      }
    }
    int args_size = list_size(*args);
    if (args_size != 1) {
      log_add(NULL, ERROR, program.name, "El valor pasado es invalido");
      log_show_and_clear(NULL);
      program.return_code = EXIT_FAILURE;
      return;
    }
    NODE_ptr new_value_node = list_index_get(0, *args);
    char *new_value = (char *)new_value_node->point;
    int result_code =
        prop_reg_type_validate(code->cdg_value.assign_prop.prop->type, new_value);
    switch (result_code) {
    case 0:
      log_add(NULL, ERROR, program.name,
              "No se encontro el tipo de dato que contiene la propiedad");
      program.return_code = EXIT_FAILURE;
      return;
    case 2:
      log_add(NULL, ERROR, program.name,
              "El formato de la entrada no coincide con el formato "
              "requerido por la propiedad");
      program.return_code = EXIT_FAILURE;
      return;
    }
    free(code->cdg_value.assign_prop.prop->value);
    free(args);
    code->cdg_value.assign_prop.prop->value = strdup(new_value);
    program.return_code = EXIT_SUCCESS;
  }
}

int argument_inter(NODE_ptr *node, char **result) {
  Raw_Argument *ra = (Raw_Argument *)(*node)->point;
  int n = concat_inter(node, ra->ra_content.argument, result);
  if (n == 0) {
    return 0;
  } else if (n == 1) {
    int result_len = strlen(ra->ra_content.argument);
    *result = malloc(result_len + 1);
    memcpy(*result, ra->ra_content.argument, result_len);
    (*result)[result_len] = '\0';
  } else if (n == 2)
    return 1;
  return 0;
}

int property_inter(NODE_ptr *node, char **result) {
  Raw_Argument *ra = (Raw_Argument *)(*node)->point;
  prop_struct *prop = prop_reg_value(ra->ra_content.prop, *prop_register);
  int n = concat_inter(node, prop->value, result);
  if (n == 0) {
    return 0;
  } else if (n == 1) {
    int result_len = strlen(prop->value);
    *result = malloc(result_len + 1);
    memcpy(*result, prop->value, result_len);
    (*result)[result_len] = '\0';
  } else if (n == 2)
    return 1;
  return 0;
}

int concat_inter(NODE_ptr *node, char *argument, char **result) {
  NODE_ptr ra_next_node = (*node)->next;
  if (ra_next_node == NULL) return 1;

  Raw_Argument *ra_next = (Raw_Argument *)ra_next_node->point;
  if (ra_next->ra_type != RAT_CONCAT) return 1;

  NODE_ptr ra_concat_node = ra_next_node->next;
  if (ra_concat_node == NULL) return 2;

  Raw_Argument *ra_concat = (Raw_Argument *)ra_concat_node->point;
  char *concat_argument = NULL;

  if (ra_concat->ra_type == RAT_ARGUMENT) {
    int n = argument_inter(&ra_concat_node, &concat_argument);
    if (n == 1) return 2;
  } else if (ra_concat->ra_type == RAT_PROPERTY) {
    int n = property_inter(&ra_concat_node, &concat_argument);
    if (n == 1) return 2;
  } else
    return 2;

  int first_len = strlen(argument);
  int second_len = strlen(concat_argument);
  int result_len = first_len + second_len;
  *result = malloc(result_len + 1);
  if (*result == NULL) return 2;

  memcpy(*result, argument, first_len);
  memcpy(*result + first_len, concat_argument, second_len);
  (*result)[result_len] = '\0';

  free(concat_argument);
  *node = ra_concat_node;
  return 0;
}
