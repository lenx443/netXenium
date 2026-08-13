/*
 * Netxenium Network Framework
 * Copyright (C) 2024-2026 Lenx443 <menasalejandro153@gmail.com>
 *
 * Este programa es software libre: usted puede redistribuirlo y/o modificarlo
 * bajo los términos de la Licencia Pública General GNU publicada por la
 * Free Software Foundation, ya sea la versión 3 de la Licencia, o
 * (a su elección) cualquier versión posterior.
 *
 * Este programa se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTÍA; incluso sin la garantía implícita de
 * COMERCIABILIDAD o IDONEIDAD PARA UN PROPÓSITO PARTICULAR. Vea la
 * Licencia Pública General GNU para más detalles.
 *
 * Usted debería haber recibido una copia de la Licencia Pública General GNU
 * junto con este programa. Si no, véase <https://www.gnu.org/licenses/>.
 */

/*
 * Netxenium Network Framework
 * Copyright (C) 2024-2026 Lenx443 <menasalejandro153@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <getopt.h>

#include "program.h"
#include "xen_life.h"

int main(int argc, char** argv) {
  if (!Xen_Init()) {
    return 1;
  }

  int c;
  int command_mode = 0;
  int parsing = 1;
  while (parsing) {
    int option_index = 0;
    static struct option long_options[] = {
      {"cmd", no_argument, 0, 'c'},
      {0, 0, 0, 0},
    };
    c = getopt_long(argc, argv, "+c", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c':
        command_mode = 1;
        break;
      case '?':
        parsing = 0;
        break;
      default:
        break;
    }
  }

  int exit_code;
  if (optind < argc) {
    if (command_mode) exit_code = Xen_Program_Run_Command_File(argc - optind, (const char**)argv + optind);
    else exit_code = Xen_Program_Run_File(argc - optind, (const char**)argv + optind);
  } else {
    if (command_mode) exit_code = Xen_Program_Run_Command_Shell();
    else exit_code = Xen_Program_Run_Repl();
  }
  Xen_Finish();
  return exit_code;
}
