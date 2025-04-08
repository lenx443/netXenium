cmake_minimum_required(VERSION 3.15)

project(ARP_Spoofer)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)



include_directories(include/)

add_executable(arp
  src/main.c
  src/functions.c
  src/handle_sniff.c
)

target_include_directories(arp PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(arp ${PCAP_LIBRARIES})
