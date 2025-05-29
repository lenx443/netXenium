#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "commands.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "terminal.h"

#define NAME "help"

static int fn_help(LIST_ptr args) {
  if (list_size(*args) == 2) {
    NODE_ptr node_cmd = list_index_get(1, *args);
    char *str_cmd = (char *)node_cmd->point;
    int i = 0;
    while (cmds_table[i] != NULL && strcmp(cmds_table[i]->name, str_cmd) != 0) {
      i++;
    }
    if (cmds_table[i] == NULL) {
      log_add(NULL, ERROR, NAME, "El comando '{%s}' no fue encontrado", str_cmd);
      log_show_and_clear(NULL);
      return 153;
    }
    printf("cmd %s:\n", cmds_table[i]->name);

    printf("\n%s\n", cmds_table[i]->short_desc);

    if (cmds_table[i]->standar_desc != NULL) { printf("\n%s\n", cmds_table[i]->standar_desc); }
    if (cmds_table[i]->long_desc != NULL) { printf("\n%s\n", cmds_table[i]->long_desc); }
    return EXIT_SUCCESS;
  }
  int max_len = 0;
  for (int i = 0; cmds_table[i] != NULL; i++) {
    int n_len = strlen(cmds_table[i]->name);
    if (n_len > max_len) max_len = n_len;
  }
  term_size tz = {0};
  if (!get_terminal_size(&tz)) {
    log_add(NULL, WARNING, NAME, "No se pudo obtener el tamaño de la terminal");
    log_show_and_clear(NULL);
  }
  int term_cols = tz.COLS - 15;
  for (int i = 0; cmds_table[i] != NULL; i++) {
    const char *standar_desc = "";
    if (cmds_table[i]->standar_desc == NULL)
      standar_desc = cmds_table[i]->short_desc;
    else
      standar_desc = cmds_table[i]->standar_desc;
    int name_printed = 0;

    char buffer[1024];
    strncpy(buffer, standar_desc, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, " ");
    char line[1024] = "";
    while (token) {
      if (strlen(line) + strlen(token) + 1 > term_cols) {
        if (!name_printed) {
          printf(VERDE "%-*s" RESET " %s\n", max_len, cmds_table[i]->name, line);
          name_printed = 1;
        } else {
          printf("%-*s  %s\n", max_len, " ", line);
        }
        line[0] = '\0';
      }

      strcat(line, token);
      strcat(line, " ");
      token = strtok(NULL, " ");
    }

    if (line[0] != '\0') {
      if (!name_printed)
        printf(VERDE "%-*s" RESET " %s\n", max_len, cmds_table[i]->name, line);
      else
        printf("%-*s %s\n", max_len, " ", line);
    }
  }
  return EXIT_SUCCESS;
}

const Command cmd_help = {
    NAME,
    "Muestra la ayuda",
    NULL,
    "Comando: help [comando]\n"
    "Descripcion:\n"
    "  Muestra información de ayuda.\n"
    "\n"
    "Uso:\n"
    "  help             - Lista todos los comandos disponibles con descripciones cortas.\n"
    "  help [comando]   - Muestra la ayuda detallada para el comando especificado.\n"
    "\n"
    "Funcionamiento:\n"
    "  - Si se pasa un comando, busca y muestra su nombre, descripción corta, descripción estándar y larga si "
    "existen.\n"
    "  - Si no se pasa comando, lista todos los comandos con sus descripciones, ajustando el texto al ancho de la "
    "terminal.\n"
    "  - Si no puede obtener el tamaño de la terminal, muestra una advertencia.\n"
    "  - Si el comando solicitado no existe, registra un error.\n",
    {0, 1},
    fn_help,
};
