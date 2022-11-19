set(thread_supervisor_TESTS "OFF"   CACHE STRING "")

# sysroot breaks find_package(), not important in this case
#set(CMAKE_SYSROOT           "${CMAKE_CURRENT_LIST_DIR}/install/" CACHE STRING "")
#set(CMAKE_INSTALL_PREFIX    "${CMAKE_SYSROOT}/usr/"                 CACHE STRING "")
set(CMAKE_INSTALL_PREFIX    "${CMAKE_BINARY_DIR}/../install/usr/" CACHE STRING "")

