cmake_minimum_required(VERSION 3.20)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# This tells CMake to look inside your src folder for the file
add_executable(example src/SISIFI_example.cc)