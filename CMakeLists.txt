cmake_minimum_required(VERSION 3.10)

project(compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PROJECT_EXE ${PROJECT_NAME})

set(backend_files
    backend/backend.hpp
    backend/backend.cpp
    backend/optimization/optimization.hpp
    backend/codegen/codegen.hpp
    backend/codegen/codegen.cpp
    backend/codegen/err.hpp
    backend/optimization/graph_color.hpp)

set(frontend_files
    frontend/express_node.hpp
    frontend/express_node.cpp
    frontend/ir_generator.hpp
    frontend/ir_generator.cpp
    frontend/symbol.hpp
    frontend/symbol_table.hpp
    frontend/symbol_table.cpp
    frontend/syntax_analyze.hpp
    frontend/syntax_analyze.cpp
    frontend/word_analyze.hpp
    frontend/word_analyze.cpp)

set(mir_files
    mir/mir.cpp
    mir/mir.hpp)

set(arm_files
    arm_code/arm.cpp
    arm_code/arm.hpp)

add_library(backend ${backend_files})
add_library(frontend ${frontend_files})
add_library(mir ${mir_files})
add_library(arm ${arm_files})

add_executable(compiler main.cpp)

if(STATIC_LINK_ARTIFACTS)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    set_target_properties(compiler PROPERTIES LINK_SEARCH_START_STATIC 1)
    set_target_properties(compiler PROPERTIES LINK_SEARCH_END_STATIC 1)
endif(STATIC_LINK_ARTIFACTS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

target_link_libraries(
    compiler
    frontend 
    backend 
    mir
    arm)
