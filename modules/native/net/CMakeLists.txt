add_library(xen_libnet
  ip/src/net_ip.c
  net.c
)

set_target_properties(xen_libnet PROPERTIES OUTPUT_NAME "net")

target_link_options(xen_libnet PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libnet PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
  ip/include
)

target_link_libraries(xen_libnet PRIVATE
  libntxenium
)

install(TARGETS xen_libnet LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
