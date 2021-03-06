cmake_minimum_required(VERSION 3.14)

#**********************
# Setup XMOS toolchain
#**********************
set(CMAKE_C_COMPILER "xcc")
set(CMAKE_AR "xmosar")
set(CMAKE_C_COMPILER_AR "xmosar")

set(CMAKE_RANLIB "")
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)

set(CMAKE_C_FLAGS "" CACHE STRING "C Compiler Base Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "" FORCE)

enable_language(C ASM)

#**********************
# Setup project
#**********************
project(mat_mul VERSION 1.0.0)

#**********************
# Disable in-source build.
#**********************
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

#**********************
# install
#**********************
set(INSTALL_DIR "${PROJECT_SOURCE_DIR}/bin")

#**********************
# Build flags
#**********************
set(BUILD_FLAGS
  "-target=XCORE-AI-EXPLORER"
  "-mcmodel=large"
  "-fxscope"
  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
  "-Wno-xcore-fptrgroup"
  "-Wno-unknown-pragmas"
  "-report"
  "-DBARE_METAL"
  "-DNDEBUG"
  "-march=xs3a"
  "-Os"
)

add_executable(mat_mul)

target_compile_options(mat_mul PRIVATE ${BUILD_FLAGS})
target_link_options(mat_mul PRIVATE ${BUILD_FLAGS})
set_target_properties(mat_mul PROPERTIES OUTPUT_NAME mat_mul.xe)

#**********************
# targets
#**********************
include("${CMAKE_CURRENT_SOURCE_DIR}/../../lib_dispatch.cmake")

set(LIB_DISPATCH_SOURCES ${LIB_DISPATCH_METAL_SOURCES})

target_sources(mat_mul
  PRIVATE ${LIB_DISPATCH_SOURCES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  PRIVATE ${MATRIX_MULTIPLY_SOURCES}
)

target_include_directories(mat_mul
  PRIVATE ${LIB_DISPATCH_INCLUDES}
  PRIVATE ${MATRIX_MULTIPLY_INCLUDES}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

install(TARGETS mat_mul DESTINATION ${INSTALL_DIR})
