cmake_minimum_required(VERSION 3.2)
#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")
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

if(${LIBRETRO_STATIC})
add_library(arduous_libretro STATIC ${INCLUDE_FILES} ${SOURCE_FILES})
else()
add_library(arduous_libretro SHARED ${INCLUDE_FILES} ${SOURCE_FILES})
endif()
if (ANDROID)
    set_target_properties(arduous_libretro PROPERTIES SUFFIX "_android.so")
elseif(EMSCRIPTEN)
    set_target_properties(arduous_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.bc")
elseif(LIBRETRO_STATIC)
    set_target_properties(arduous_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.a")
endif ()

target_include_directories(
    arduous_libretro
    PUBLIC include
    PUBLIC ${CMAKE_SOURCE_DIR}/simavr/simavr/sim
    PUBLIC ${CMAKE_SOURCE_DIR}/simavr/simavr/cores
    PUBLIC ${CMAKE_SOURCE_DIR}/simavr/examples/parts
)

set_property(TARGET arduous_libretro PROPERTY C_STANDARD 99)

target_sources(arduous_libretro PRIVATE
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_acomp.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_adc.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_bitbang.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_eeprom.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_extint.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_flash.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_ioport.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_lin.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_spi.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_timer.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_twi.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_uart.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_usb.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/avr_watchdog.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_avr.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_cmds.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_core.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_cycle_timers.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_hex.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_interrupts.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_io.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_irq.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_utils.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/sim/sim_vcd_file.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega1280.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega1281.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega1284.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega128.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega128rfa1.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega128rfr2.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega164.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega168.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega169.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega16.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega16m1.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega2560.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega324a.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega324.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega328.c
#  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega328pb.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega32.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega32u4.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega48.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega644.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega64m1.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega88.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_mega8.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_megax4.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_megax8.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_megax.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_megaxm1.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny13.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny2313a.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny2313.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny24.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny25.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny4313.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny44.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny45.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny84.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tiny85.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tinyx4.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_tinyx5.c
  ${CMAKE_SOURCE_DIR}/simavr/simavr/cores/sim_usb162.c

  ${CMAKE_SOURCE_DIR}/simavr/examples/parts/ssd1306_virt.c

  ${CMAKE_SOURCE_DIR}/src/sim_fake_gdb.c
  )

# Include a strsep for some platforms where it's missing (Windows, maybe others)
include(CheckSymbolExists)
check_symbol_exists(strsep "string.h" HAVE_STRSEP)
if (NOT HAVE_STRSEP)
    target_sources(arduous_libretro PRIVATE ${CMAKE_SOURCE_DIR}/src/strsep.c)
endif ()

if (LIBRETRO)
  execute_process(COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if ("${GIT_VERSION}" STREQUAL "")
    set(GIT_VERSION "unknown")
  endif ()
  add_definitions(-DGIT_VERSION=" ${GIT_VERSION}")
endif ()

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

# Enable all warnings except for no-sign-compare, which Clang disables
# by default.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare")

option(WERROR "Turn warnings into errors" ON)
if (WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()
