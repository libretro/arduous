include(ExternalProject)

find_program(MAKE_EXE NAMES gmake nmake make)

ExternalProject_Add(
    simavr
    PREFIX simavr
    GIT_REPOSITORY https://github.com/buserror/simavr.git
    GIT_SHALLOW true
    GIT_TAG v1.7
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${MAKE_EXE} build-simavr build-parts
    BUILD_IN_SOURCE true
    INSTALL_COMMAND ""
    PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/external/patches/simvar/0001_makefile_common.patch ${CMAKE_SOURCE_DIR}/external/patches/simvar/0002_extern.patch || git apply ${CMAKE_SOURCE_DIR}/external/patches/simvar/0001_makefile_common.patch ${CMAKE_SOURCE_DIR}/external/patches/simvar/0002_extern.patch --reverse --check && echo already applied
)
