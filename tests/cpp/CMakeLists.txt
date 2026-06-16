add_executable(run_tests test_main.cpp)
target_link_libraries(run_tests PRIVATE suffixtree_core)
target_compile_options(run_tests PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME cpp_tests COMMAND run_tests)
