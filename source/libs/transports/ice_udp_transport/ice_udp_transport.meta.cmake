include("${CMAKE_CURRENT_LIST_DIR}/../../../../cmake_modules/libnice.cmake")

set(LIB_EXTRA_LIBS udp_transport Libnice::Nice)
