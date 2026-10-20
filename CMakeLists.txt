cmake_minimum_required(VERSION 3.18)
project(dfzygisk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(zygisk SHARED src/module.cpp)

target_compile_options(zygisk PRIVATE -fno-exceptions -fno-rtti -fvisibility=hidden -O2 -Wall)

target_link_libraries(zygisk PRIVATE log dl)

set_target_properties(zygisk PROPERTIES PREFIX "" OUTPUT_NAME "zygisk")
