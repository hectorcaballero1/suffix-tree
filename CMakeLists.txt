cmake_minimum_required(VERSION 3.18)
project(suffix_tree CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Python + pybind11 ────────────────────────────────────────────────────────
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

if(NOT DEFINED pybind11_DIR OR pybind11_DIR STREQUAL "")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_DIR
        RESULT_VARIABLE pybind11_RESULT
        ERROR_VARIABLE  pybind11_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT pybind11_RESULT EQUAL 0)
        message(FATAL_ERROR "pybind11 not found: ${pybind11_ERROR}")
    endif()
endif()

find_package(pybind11 CONFIG REQUIRED)

add_subdirectory(core)
add_subdirectory(tests/cpp)
