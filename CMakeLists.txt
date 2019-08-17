cmake_minimum_required(VERSION 3.14)
project(luaplusplus)

set(CMAKE_CXX_STANDARD 17)
set(FILES src/main.cpp)
set(PYTHON_EXECUTABLE python3.7)
set(LUA_LIBRARIES lua53)
set(LUA_INCLUDE_PATH lib/lua)
find_package(PythonInterp)
find_package(PythonLibs)
include_directories(${PYTHON_INCLUDE_PATH} ${LUA_INCLUDE_PATH})

add_library(luaplusplus SHARED ${FILES})
target_link_libraries(luaplusplus ${LUA_LIBRARIES})