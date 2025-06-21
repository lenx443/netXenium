#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "lexer.h"
#include "list.h"
#include "logs.h"
#include "parser.h"
#include "program.h"
#include "properties.h"
#include "raw_arguments.h"

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

int parser_stmt(Parser *p, Parser_Code *code) {
  if (p->token.tkn_type == TKN_EOF) {
    code->cdg_type = CDG_NULL;
  } else if (p->token.tkn_type == TKN_COMMAND) {
    code->cdg_type = CDG_COMMAND;
    int match = 0;
    for (int i = 0; cmds_table[i] != NULL; i++) {
      if (strcmp(cmds_table[i]->name, p->token.tkn_text) == 0) {
        code->cdg_value.command.command = (Command *)cmds_table[i];
        match = 1;
      }
    }
    if (!match) {
      log_add(NULL, ERROR, program.name, "{%s} no fue reconosido", p->token.tkn_text);
      log_show_and_clear(NULL);
      return 0;
    }
    char *cmd_name = strdup(code->cdg_value.command.command->name);
    parser_next(p);
    code->cdg_value.command.args = list_new();
    if (code->cdg_value.command.args == NULL) {
      log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
      log_show_and_clear(NULL);
      return 0;
    }
    if (!raw_args_push_argument(code->cdg_value.command.args, cmd_name)) {
      log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
      log_show_and_clear(NULL);
      return 0;
    }
    while (p->token.tkn_type != TKN_EOF) {
      switch (p->token.tkn_type) {
      case TKN_ARGUMENT:
      case TKN_STRING: {
        if (!raw_args_push_argument(code->cdg_value.command.args, p->token.tkn_text)) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          return 0;
        }
        parser_next(p);
        break;
      }
      case TKN_PROPERTY: {
        if (prop_reg_search_key(p->token.tkn_text, *prop_register) == -1) {
          log_add(NULL, ERROR, program.name, "la propiedad {%s} no existe",
                  p->token.tkn_text);
          log_show_and_clear(NULL);
          return 0;
        }
        if (!raw_args_push_property(code->cdg_value.command.args, p->token.tkn_text)) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          return 0;
        }
        parser_next(p);
        break;
      }
      case TKN_CONCAT: {
        if (!raw_args_push_concat(code->cdg_value.command.args)) {
          log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
          log_show_and_clear(NULL);
          return 0;
        }
        parser_next(p);
        break;
      }
      default: {
        log_add(NULL, ERROR, program.name, "{%s} no es argumento valido",
                p->token.tkn_text);
        log_show_and_clear(NULL);
        return 0;
      }
      }
    }
    free(cmd_name);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    char *prop_name = strdup(p->token.tkn_text);
    parser_next(p);
    if (p->token.tkn_type == TKN_EQUAL) {
      code->cdg_type = CDG_ASIGN_PROP;
      prop_struct *prop = prop_reg_value(prop_name, *prop_register);
      if (prop == NULL) {
        log_add(NULL, ERROR, program.name, "la propiedad {%s} no existe", prop_name);
        log_show_and_clear(NULL);
        return 0;
      }
      parser_next(p);
      code->cdg_value.assign_prop.prop = prop;
      code->cdg_value.assign_prop.new_value = list_new();
      if (code->cdg_value.command.args == NULL) {
        log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
        log_show_and_clear(NULL);
        return 0;
      }
      while (p->token.tkn_type != TKN_EOF) {
        switch (p->token.tkn_type) {
        case TKN_ARGUMENT:
        case TKN_STRING: {
          if (!raw_args_push_argument(code->cdg_value.assign_prop.new_value,
                                      p->token.tkn_text)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            return 0;
          }
          parser_next(p);
          break;
        }
        case TKN_PROPERTY: {
          if (prop_reg_search_key(p->token.tkn_text, *prop_register) == -1) {
            log_add(NULL, ERROR, program.name, "la propiedad {%s} no existe",
                    p->token.tkn_text);
            log_show_and_clear(NULL);
            return 0;
          }
          if (!raw_args_push_property(code->cdg_value.assign_prop.new_value,
                                      p->token.tkn_text)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            return 0;
          }
          parser_next(p);
          break;
        }
        case TKN_CONCAT: {
          if (!raw_args_push_concat(code->cdg_value.assign_prop.new_value)) {
            log_add(NULL, ERROR, program.name, "No se pudo alistar los argumentos");
            log_show_and_clear(NULL);
            return 0;
          }
          parser_next(p);
          break;
        }
        default: {
          log_add(NULL, ERROR, program.name, "{%s} no es una exprecion valida",
                  p->token.tkn_text);
          log_show_and_clear(NULL);
          return 0;
        }
        }
      }
    }
    free(prop_name);
  } else {
    log_add(NULL, ERROR, program.name, "{%s} no fue reconosido", p->token.tkn_text);
    log_show_and_clear(NULL);
    return 0;
  }
  return 1;
}

void parser_code_free(Parser_Code *code) {
  if (code == NULL) return;
  if (code->cdg_type == CDG_COMMAND) { raw_args_free(code->cdg_value.command.args); }
}
