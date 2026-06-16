#pragma once
#include <string>
#include <vector>

// Brute-force O(n*m) substring search.
// Used as the correctness oracle for Ukkonen tests and as the benchmark baseline.
std::vector<int> naive_search(const std::string& text, const std::string& pattern);
