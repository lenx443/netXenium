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

#include "compiler.h"
#include "interpreter.h"
#include "program.h"
#include "vm.h"
#include "xen_cstrings.h"
#include "xen_life.h"

int main(int argc, char** argv) {
  if (!Xen_Init()) {
    return 1;
  }

  int c;
  int shell_mode = 0;
  char *simple_line = NULL;
  int parsing = 1;
  while (parsing) {
    int option_index = 0;
    static struct option long_options[] = {
      {"shell", no_argument, 0, 's'},
      {"cmd", required_argument, 0, 'c'},
      {0, 0, 0, 0},
    };
    c = getopt_long(argc, argv, "+sc:", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 's':
        shell_mode = 1;
        break;
      case 'c':
        simple_line = Xen_CString_Dup(optarg);
        break;
      case '?':
        parsing = 0;
        break;
      default:
        break;
    }
  }

  int exit_code;
  if (simple_line) {
    if (shell_mode) {
      Xen_Program_Push(0, NULL);
      exit_code = Xen_Program_Run_Command(simple_line);
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
      }
      Xen_Program_Pop();
    } else {
      Xen_Program_Push(0, NULL);
      if (!interpreter("<only-line>", simple_line, Xen_COMPILE_PROGRAM, NULL, NULL, NULL)) {
        if (Xen_VM_Except_Active()) {
          Xen_VM_Except_Backtrace_Show();
        }
      }
      exit_code = Xen_Program_Pop();
    }
  } else if (optind < argc) {
    if (shell_mode) exit_code = Xen_Program_Run_Command_File(argc - optind, (const char**)argv + optind);
    else exit_code = Xen_Program_Run_File(argc - optind, (const char**)argv + optind);
  } else {
    if (shell_mode) exit_code = Xen_Program_Run_Command_Shell();
    else exit_code = Xen_Program_Run_Repl();
  }
  Xen_Finish();
  return exit_code;
}
