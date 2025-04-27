/*
 * Este comando tiene como objetivo principal
 * abstraer ciertas tareas en redes locales.
 * Actualmente este permite hacer ARP Spoofing
 * en su red local.
 * (C) Lenx443 2024-2025
 */

// Mensaje de error para dispositivo Wundiws.
#if defined(__WIN32) || defined(__WIN64)
#warning "Este programa no es compatible aun con su dispositivo"
#warning "Para mas informacion puede ir a https://github.com/lenx443"
#error "Dispositivo no compatible"
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "colors.h"
#include "commands.h"
#include "config.h"
#include "errs.h"
#include "list.h"
#include "program.h"

#define push_back(list, value, size)                                           \
  if (!list_push_back(list, value, size))                                      \
    return -1;

#define push_string(list, string)                                              \
  if (!list_push_back_string_node(list, string))                               \
    return 0;

int main(int argc, char **argv) {
  signal(SIGINT, no_signal);

  // Aqui se inicializan las
  // propiedades de la shell.
  config = config_new();
  if (config == NULL) {
    print_error(argv[0], "No se pudo iniciar la configuracion");
    return 1;
  }
  printf(AZUL "NetXenium" RESET " (C) " AMARILLO "Lenx443 2024-2025" RESET "\n"
              "Type " VERDE "help" RESET " for more info\n");

  /*
   * Este es el bucle principa del proograma.
   * donde se manejan los comandos y sus retornos
   * (la shell).
   */
  int return_code = 0;
  while (1) {
    // Aqui se reciven y validan las entradas.
    char cmd[1024];
    printf("[%s%d" RESET "] " AMARILLO "%s" RESET " > ",
           return_code == 0 ? VERDE : ROJO, return_code, argv[0]);
    if (fgets(cmd, 1024, stdin) == NULL) {
      putc('\n', stdout);
      break;
    }
    cmd[strcspn(cmd, "\n")] = '\0';

    // Se tokenisan los argumentos
    LIST_ptr args = list_new();
    char *token = strtok(cmd, " ");
    while (token != NULL) {
      push_string(args, token);
      token = strtok(NULL, " ");
    }
    if (list_size(*args) == 0) {
      list_free(args);
      continue;
    }

    /*
     * Se verific aque el comando este
     * disponible y obtiene su retorno.
     */
    NODE_ptr cmd_node = list_index_get(0, *args);
    if (cmd_node == NULL) {
      list_free(args);
      continue;
    }
    char *cmd_str = (char *)cmd_node->point;
    int matched = 0;
    for (int i = 0; cmds[i] != NULL; i++) {
      if (strcmp(cmds[i]->name, cmd_str) == 0) {
        return_code = cmds[i]->func(args);
        matched = 1;
      }
    }
    if (!matched) {
      printf("Not Command matched: " ROJO "%s" RESET "\n", cmd_str);
      return_code = EXIT_FAILURE;
    }

    /*
     * Si algun comando solicita
     * terminar la shell se para el
     * bucle.
     */
    if (program.closed)
      break;
    list_free(args);
  }

  /*
   * Al finalizar el bucle se libera
   * la memoria.
   */
  config_free(config);
  return 0;
}
