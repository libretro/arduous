cmake_minimum_required(VERSION 3.2)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
project(arduous VERSION 0.1.0)

# Require C++11.
# Change this value to require other C++ standards (14, 17, etc.)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link.T")

# Use globbing to find source files.
# Any source files added to include/ or src/ will automatically be added to the project.
file(GLOB_RECURSE INCLUDE_FILES ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
# file(GLOB_RECURSE SOURCE_FILES_NO_MAIN ${CMAKE_SOURCE_DIR}/src/*.cpp)
# list(REMOVE_ITEM SOURCE_FILES_NO_MAIN ${CMAKE_SOURCE_DIR}/src/main.cpp)
# set(SOURCE_FILES ${SOURCE_FILES_NO_MAIN} ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_library(arduous_libretro SHARED ${INCLUDE_FILES} ${SOURCE_FILES})

find_library(LIB_ELF elf)

add_dependencies(arduous_libretro simavr)

target_include_directories(
    arduous_libretro
    PUBLIC include
    PUBLIC ${CMAKE_BINARY_DIR}/simavr/src/simavr/simavr/sim
    PUBLIC ${CMAKE_BINARY_DIR}/simavr/src/simavr/examples/parts
)

target_link_libraries(
    arduous_libretro
    ${CMAKE_BINARY_DIR}/simavr/src/simavr/simavr/obj/libsimavr.a
    ${CMAKE_BINARY_DIR}/simavr/src/simavr/examples/parts/obj/libsimavrparts.a
    ${CMAKE_BINARY_DIR}/simavr/src/simavr/examples/parts/obj/ssd1306_virt.o
    ${LIB_ELF}
)

set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT}")
set_target_properties(arduous_libretro PROPERTIES PREFIX "")
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/libretro/libretro.cpp PROPERTIES OBJECT_DEPENDS ${LINKER_SCRIPT})

# If external libraries are needed, use find_library() and target_link_libraries()
# to dynamically link them to your executable. Generally, you'll want to install
# them onto the host using your OS's package manager.
# Example:
#   find_library(LIB_FMT fmt)
#   target_link_libraries(clang-blueprint ${LIB_FMT})

# Include extra CMake files that are used for testing and linting.
# include(${CMAKE_SOURCE_DIR}/cmake/unit_test.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/clang_tools.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/cppcheck.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/simavr.cmake)

# Enable all warnings except for no-sign-compare, which Clang disables
# by default.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare")

option(WERROR "Turn warnings into errors" ON)
if (WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()
