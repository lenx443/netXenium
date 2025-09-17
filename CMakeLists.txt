cmake_minimum_required(VERSION 3.15)

project(NetXenium)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer -g -O0 -Wall")

set(BUILD_SHARED_LIBS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)

set(C_STANDARD 99)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Enabling Address Sanitizer")
  link_libraries(-fsanitize=address)
endif()

include_directories(
  core/include/
  core/common/include/
  core/common/instances/basic/include/
  core/common/instances/command/include/
  core/common/instances/command/types/header/
  core/common/instances/string/include/
  core/common/instances/string/types/header/
  core/common/instances/number/include/
  core/common/instances/number/types/header/
  core/common/instances/vector/include/
  core/common/instances/vector/type/header/
  core/common/instances/nil/include/
  core/common/instances/map/include/
  core/common/instances/map/types/header/
  core/common/instances/boolean/include/
  core/common/instances/boolean/types/header/
  core/interpreter/include/
  core/interpreter/dynamic/vectors/include/
  core/interpreter/dynamic/trees/include/
  core/interpreter/compiler/include/
  core/interpreter/compiler/types/header/
  core/interpreter/compiler/dynamic/lists/include/
  core/interpreter/compiler/dynamic/vectors/include/
  core/virtual_machine/include/
  core/virtual_machine/types/include/
  core/virtual_machine/dynamic/vectors/include/
  core/virtual_machine/dynamic/lists/include/
  core/instances/include
  core/instances/types/include/
  core/register_enginer/include/
  core/register_enginer/types/header/
  core/modules_enginer/include/
  core/modules_enginer/types/header/
  core/modules_enginer/instance/include/
  core/modules_enginer/instance/types/header/
  modules/include/
  api/include/
  dynamic/lists/include/
  dynamic/hash_tables/include/
  header/
)

set(SOURCES
  # Codigo fuente donde del nucleo
  # del proyecto.
  core/src/xen_life.c
  core/src/properties_types.c
  core/src/program.c
  core/src/read_string_utf8.c

  # Codigo Commun
  core/common/src/operators.c
  core/common/src/callable.c

  # Implementacion del objeto `Basic`
  core/common/instances/basic/src/basic.c

  # Implementacion del objeto `Command`
  core/common/instances/command/src/xen_command_implement.c
  core/common/instances/command/src/xen_command.c

  # Implementacion del objeto `String`
  core/common/instances/string/src/xen_string_implement.c
  core/common/instances/string/src/xen_string.c

  # Implementacion del objeto `Number`
  core/common/instances/number/src/xen_number_implement.c
  core/common/instances/number/src/xen_number.c

  # Implementacion del objeto `Vector`
  core/common/instances/vector/src/xen_vector_implement.c
  core/common/instances/vector/src/xen_vector.c

  # Implementacion del objeto `Nil`
  core/common/instances/nil/src/xen_nil_implement.c
  core/common/instances/nil/src/xen_nil.c

  # Implementacion del objeto `Map`
  core/common/instances/map/src/xen_map_implement.c
  core/common/instances/map/src/xen_map.c

  # Implementacion del objeto `Boolean`
  core/common/instances/boolean/src/xen_boolean_implement.c
  core/common/instances/boolean/src/xen_boolean.c

  # Codigo que se encarga de procesar
  # e interpretar scripts
  core/interpreter/src/interpreter.c
  core/interpreter/src/lexer.c
  core/interpreter/src/parser.c

  # Implementaciones de los codigos
  # fuente de los arboles dinamicos
  # del interpreter
  core/interpreter/dynamic/trees/src/ast.c

  # Implementaciones de los codigos
  # fuente de los vectores del
  # interpreter
  core/interpreter/dynamic/vectors/src/ast_array.c

  # Compilador a bytecode.
  # [AST Tree]
  #    |
  # [IR bytecode]
  #    |
  # [bytecode]
  core/interpreter/compiler/src/ast_compiler.c
  core/interpreter/compiler/src/blocks_linealizer.c
  core/interpreter/compiler/src/blocks_compiler.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de el compilador
  core/interpreter/compiler/dynamic/lists/src/block_list.c

  # Implementaciones de los codigos
  # fuente de los vectores de el
  # compilador
  core/interpreter/compiler/dynamic/vectors/src/ir_bytecode.c

  # Entorno de maquina virtual para la
  # ejecucion de bytecode.
  core/virtual_machine/src/vm.c
  core/virtual_machine/src/vm_run.c

  # Tipos definidos en la maquina virtual
  core/virtual_machine/types/src/vm_def.c
  core/virtual_machine/types/src/vm_register.c
  core/virtual_machine/types/src/run_ctx.c
  core/virtual_machine/types/src/run_frame.c

  # Implementaciones de los codigos
  # fuente de los vectores de la
  # maquina virtual
  core/virtual_machine/dynamic/vectors/src/bytecode.c
  core/virtual_machine/dynamic/vectors/src/vm_consts.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de la maquina virtual
  core/virtual_machine/dynamic/lists/src/run_ctx_stack.c

  # Codigo de Instancias
  core/instances/src/instance_life.c

  # Implementacion de estaructura `Instance`
  core/instances/types/src/implement.c
  core/instances/types/src/instance.c

  # motor de registros
  core/register_enginer/src/xen_register.c

  # motor de modulos
  core/modules_enginer/src/xen_modules_def.c
  core/modules_enginer/src/xen_module_load.c

  # Instanca del motor de modulos
  core/modules_enginer/instance/src/xen_module_implement.c
  core/modules_enginer/instance/src/xen_module.c

  # modulos
  modules/src/m_core.c

  # codigo fuente de la la API
  # del Framework.
  api/src/functions.c
  api/src/pcap_wrapper.c
  api/src/terminal.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  dynamic/lists/src/list.c
  dynamic/lists/src/properties.c
  dynamic/lists/src/history.c
  dynamic/lists/src/suggestion.c
  dynamic/lists/src/logs.c
  dynamic/lists/src/string_utf8.c

  # Implementaciones de los codigos
  # fuente de las tablas hash
  dynamic/hash_tables/src/string_hash_table.c

  # Implementaciones de los codigo fuente
  # de los comandos definidos en
  # core/include/commands.h
  commands/help.c
  commands/exit.c
  commands/new.c
  commands/del.c
  commands/set.c
  commands/get.c
  commands/echo.c
  commands/input.c
  commands/clear_history.c
  commands/resolve.c
  commands/arp_spoof.c
  commands/iface.c
)

add_executable(ntxenium
  # Codigo fuente principal (Funcion main)
  main.c
  ${SOURCES}
)

target_include_directories(ntxenium PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(ntxenium PRIVATE ${PCAP_LIBRARIES})

enable_testing()

add_executable(test_number
  tests/test_number.c ${SOURCES}
)
target_include_directories(test_number PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_number PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestNumber COMMAND test_number)

add_executable(test_string
  tests/test_string.c ${SOURCES}
)
target_include_directories(test_string PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_string PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestString COMMAND test_string)

add_executable(test_map
  tests/test_map.c ${SOURCES}
)
target_include_directories(test_map PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_map PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestMap COMMAND test_map)

add_executable(test_boolean
  tests/test_boolean.c ${SOURCES}
)
target_include_directories(test_boolean PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_boolean PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestBoolean COMMAND test_boolean)

add_executable(test_modules
  tests/test_modules.c ${SOURCES}
)
target_include_directories(test_modules
PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_modules  PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestModules COMMAND test_modules)

add_executable(test_load_script
  tests/test_load_script.c ${SOURCES}
)
target_include_directories(test_load_script
PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(test_load_script  PRIVATE ${PCAP_LIBRARIES})
add_test(NAME TestLoadScript COMMAND test_load_script)
