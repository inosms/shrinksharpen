set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build/ )
cmake_minimum_required (VERSION 2.6)
project(shrinksharpen)

set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -L/opt/X11/lib")

# https://stackoverflow.com/questions/17006245/mac-osx-lion-x11-cimg-library
include_directories(/opt/X11/include)

add_executable(shrinksharpen main.cpp)

target_link_libraries( shrinksharpen X11 jpeg )
