cmake_minimum_required(VERSION 3.14)
project(luaplusplus)

set(CMAKE_CXX_STANDARD 17)
set(FILES src/main.cpp)
set(PYTHON_EXECUTABLE "python3.7")
find_package(PythonInterp)
find_package(PythonLibs)
find_package(Lua REQUIRED)
include_directories(${PYTHON_INCLUDE_PATH})

add_library(luaplusplus SHARED ${FILES})
target_link_libraries(luaplusplus ${LUA_LIBRARIES})