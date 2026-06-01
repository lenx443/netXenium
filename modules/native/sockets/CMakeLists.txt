add_library(xen_libsockets
  sockets.c
  socket/src/socket.c
  asocket/src/asocket.c
)

set_target_properties(xen_libsockets PROPERTIES OUTPUT_NAME "sockets")

target_link_options(xen_libsockets PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libsockets PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
  socket/include/
  asocket/include/
)

target_link_libraries(xen_libsockets PRIVATE
  libntxenium
)

install(TARGETS xen_libsockets LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
