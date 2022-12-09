cmake_minimum_required(VERSION 2.8.12)
project(pycount)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pybind11)
pybind11_add_module(pycount ${CMAKE_CURRENT_SOURCE_DIR}/src/flagser_count_bindings.cpp)
