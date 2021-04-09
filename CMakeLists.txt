cmake_minimum_required(VERSION 2.8.12)
project(pycount)
add_subdirectory(./pyflagsercount/pybind11)
pybind11_add_module(pycount ./src/flagser_count_bindings.cpp)
