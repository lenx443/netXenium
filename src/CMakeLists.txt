cmake_minimum_required(VERSION 3.15)

project(NetXenium)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

set(CMAKE_C_FLAGS_DEBUG "-g -O0")

include_directories(include/)
include_directories(header/)

add_executable(xenium
  src/main.c
  src/functions.c
  src/list.c
  src/config.c
  src/config_types.c
  src/errs.c
  src/program.c
  src/pcap_wrapper.c

  # Comandos
  src/commands/help.c
  src/commands/exit.c
  src/commands/set.c
  src/commands/get.c
  src/commands/resolve.c
  src/commands/arp_spoof.c
)

target_include_directories(xenium PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(xenium ${PCAP_LIBRARIES})
