cmake_minimum_required(VERSION 3.15)

project(NetXenium)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)
pkg_check_modules(JSONC REQUIRED json-c)

set(CMAKE_C_FLAGS_DEBUG "-g -O0")

set(C_STANDARD 99)

include_directories(
  include/
  dynamic_lists/include/
  header/
)

add_executable(ntxenium
  # Codigo fuente principal (Funcion main)
  main.c

  # Codigo fuente donde se implementan
  # las funciones definidas en include/
  src/functions.c
  src/properties_types.c
  src/program.c
  src/pcap_wrapper.c
  src/terminal.c

  # Implementaciones de los codigos
  # fuente de las librerias dinamicas
  # definidas en dynamic_lists/include/
  dynamic_lists/src/list.c
  dynamic_lists/src/properties.c
  dynamic_lists/src/history.c
  dynamic_lists/src/suggestion.c
  dynamic_lists/src/logs.c

  # Implementaciones de los codigo fuente
  # de los comandos definidos en
  # include/commands.h
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

target_include_directories(ntxenium PRIVATE ${PCAP_INCLUDE_DIRS} ${JSONC_INCLUDE_DIRS})
target_link_libraries(ntxenium ${PCAP_LIBRARIES} ${JSONC_LIBRARIES})
