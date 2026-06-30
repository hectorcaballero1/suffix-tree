# ── Static library (shared between the pybind module and the test binary) ────
add_library(suffixtree_core STATIC
    src/naive_search.cpp
    src/suffix_tree.cpp
    src/corpus_tree.cpp
)
target_include_directories(suffixtree_core PUBLIC include)
set_target_properties(suffixtree_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_options(suffixtree_core PRIVATE -Wall -Wextra)

# ── pybind11 module ───────────────────────────────────────────────────────────
pybind11_add_module(suffix_tree_core_py bindings/bindings.cpp)
target_link_libraries(suffix_tree_core_py PRIVATE suffixtree_core)
set_target_properties(suffix_tree_core_py PROPERTIES
    OUTPUT_NAME suffix_tree_core
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/backend"
)
