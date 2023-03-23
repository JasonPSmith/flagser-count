cmake_minimum_required(VERSION 3.9)
project(pycount LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.4.0")
    message(FATAL_ERROR "Insufficient gcc version")
  endif()
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pybind11)

set(BINDINGS_DIR "src")

pybind11_add_module(pycount "${BINDINGS_DIR}/flagser_count_bindings.cpp")

target_include_directories(pycount PRIVATE .)

if(MSVC)
    target_compile_options(pycount PUBLIC $<$<CONFIG:RELEASE>: /Wall /O2>)
    target_compile_options(pycount PUBLIC $<$<CONFIG:DEBUG>:/O1 /DEBUG:FULL /Zi /Zo>)
else()
    target_compile_options(pycount PUBLIC $<$<CONFIG:RELEASE>: -Ofast>)
    target_compile_options(pycount PUBLIC $<$<CONFIG:DEBUG>: -O2 -ggdb -D_GLIBCXX_DEBUG>)
endif()
