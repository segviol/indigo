cmake_minimum_required(VERSION 3.10)

project(compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PROJECT_EXE ${PROJECT_NAME})

set(backend_files
    backend/backend.hpp
    backend/backend.cpp
    backend/optimization/optimization.hpp
    backend/optimization/graph_color.hpp
    backend/optimization/livevar_analyse.hpp
    backend/optimization/remove_dead_code.hpp
    backend/codegen/codegen.hpp
    backend/codegen/codegen.cpp
    backend/codegen/bb_rearrange.cpp
    backend/codegen/reg_alloc.cpp
    backend/codegen/math_opt.cpp
    backend/codegen/err.hpp
    backend/optimization/graph_color.hpp)

set(frontend_files
    frontend/express_node.hpp
    frontend/express_node.cpp
    frontend/ir_generator.hpp
    frontend/ir_generator.cpp
    frontend/optim_mir.hpp
    frontend/optim_mir.cpp
    frontend/symbol.hpp
    frontend/symbol.cpp
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

add_library(src ${backend_files} ${frontend_files} ${mir_files} ${arm_files})

add_subdirectory(include/spdlog)

add_executable(compiler main.cpp)

if(STATIC_LINK_ARTIFACTS)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -g")
    set_target_properties(compiler PROPERTIES LINK_SEARCH_START_STATIC 1)
    set_target_properties(compiler PROPERTIES LINK_SEARCH_END_STATIC 1)
endif(STATIC_LINK_ARTIFACTS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

target_include_directories(src PRIVATE include/spdlog/include)

target_link_libraries(
    compiler
    spdlog::spdlog
    src)
