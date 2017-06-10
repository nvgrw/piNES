cmake_minimum_required (VERSION 2.6)
project (nes)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -D_POSIX_SOURCE -D_BSD_SOURCE -D_THREAD_SAFE -std=c99 -Werror -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

file (GLOB SOURCES "src/*.c" "include/*.h")

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

set (NFD_INCLUDE_ROOT ${PROJECT_SOURCE_DIR}/cmake/nativefiledialog/src)

if(APPLE)
  set (FRAMEWORKS "-framework Cocoa" "-framework AppKit" "-framework OpenGL")
endif()

include_directories (include ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
  ${NFD_INCLUDE_ROOT} ${NFD_INCLUDE_ROOT}/include)

add_subdirectory(cmake)

add_executable(nes "${SOURCES}")
target_link_libraries(nes ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} nfd ${FRAMEWORKS})
