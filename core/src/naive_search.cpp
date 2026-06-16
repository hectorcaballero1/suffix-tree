#include "naive_search.hpp"

std::vector<int> naive_search(const std::string& text, const std::string& pattern) {
    std::vector<int> result;
    if (pattern.empty() || pattern.size() > text.size()) return result;
    const int n = static_cast<int>(text.size());
    const int m = static_cast<int>(pattern.size());
    for (int i = 0; i <= n - m; ++i) {
        if (text.compare(i, m, pattern) == 0)
            result.push_back(i);
    }
    return result;
}
