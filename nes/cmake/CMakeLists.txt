cmake_minimum_required (VERSION 2.6)
project (nfd)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -m64 -O2 -fno-exceptions")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -MMD -MP")

set (SRCDIR "${PROJECT_SOURCE_DIR}/nativefiledialog/src")

set (COMMSRC "${SRCDIR}/nfd_common.c")

if (WIN32)
  set (PLATSRC ${SRCDIR}/nfd_win.cpp)
endif()

if (APPLE)
  set (PLATSRC ${SRCDIR}/nfd_cocoa.m)
endif()

if (UNIX AND NOT APPLE)
  set (PLATSRC ${SRCDIR}/nfd_gtk.c)
endif()

include_directories (${SRCDIR} ${SRCDIR}/include)
add_library (nfd STATIC ${COMMSRC} ${PLATSRC})
