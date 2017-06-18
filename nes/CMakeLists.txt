cmake_minimum_required (VERSION 2.6)
project (nes)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

option (IS_PI "IS_PI" OFF)
option (TCP_HOST "TCP_HOST" ON)

if(IS_PI)
  set (EXTRA_FLAGS "-DIS_PI")
endif()

if(TCP_HOST)
  set (EXTRA_FLAGS "${EXTRA_FLAGS} -DTCP_HOST")
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -D_THREAD_SAFE ${EXTRA_FLAGS} -std=c99 -Werror -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

file (GLOB SOURCES "src/*.c" "include/*.h")

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

set (NFD_INCLUDE_ROOT ${PROJECT_SOURCE_DIR}/cmake/nativefiledialog/src)

if(APPLE)
  set (FRAMEWORKS "-framework Cocoa" "-framework AppKit" "-framework OpenGL")
endif()

if(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  add_definitions(${GTK3_CFLAGS_OTHER})
  link_directories(${GTK3_LIBRARY_DIRS})
endif()

if(IS_PI)
  set (FRAMEWORKS ${FRAMEWORKS} pigpio)
endif()

include_directories (include ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
  ${NFD_INCLUDE_ROOT} ${NFD_INCLUDE_ROOT}/include ${GTK3_INCLUDE_DIRS})

add_subdirectory(cmake)

add_executable(nes "${SOURCES}")
target_link_libraries(nes ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} nfd ${GTK3_LIBRARIES} ${FRAMEWORKS})
