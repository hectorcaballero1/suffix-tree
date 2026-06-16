#include <cassert>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "naive_search.hpp"
#include "suffix_tree.hpp"
#include "corpus_tree.hpp"

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::vector<int> sorted(std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}

#define CHECK_EQ(a, b, label)                                          \
    do {                                                               \
        auto _a = sorted(a); auto _b = sorted(b);                     \
        if (_a != _b) {                                                \
            std::cerr << "[FAIL] " << (label) << "\n";                \
            std::cerr << "  expected: "; for (int x:_b) std::cerr<<x<<" "; std::cerr<<"\n"; \
            std::cerr << "  got:      "; for (int x:_a) std::cerr<<x<<" "; std::cerr<<"\n"; \
            return false;                                              \
        }                                                              \
        std::cout << "[OK]   " << (label) << "\n";                    \
    } while(0)

// ── Group 1: NaiveSearch ─────────────────────────────────────────────────────

static bool test_naive() {
    std::cout << "\n=== GROUP 1: NaiveSearch (oracle) ===\n";

    CHECK_EQ(naive_search("abcde", "bc"),    (std::vector<int>{1}),       "single occurrence");
    CHECK_EQ(naive_search("abcde", "xyz"),   (std::vector<int>{}),        "pattern absent");
    CHECK_EQ(naive_search("aaaa",  "a"),     (std::vector<int>{0,1,2,3}), "all same - single char");
    CHECK_EQ(naive_search("aaaa",  "aa"),    (std::vector<int>{0,1,2}),   "all same - overlapping");
    CHECK_EQ(naive_search("abcde", "abcde"), (std::vector<int>{0}),       "pattern equals text");
    CHECK_EQ(naive_search("xyzabc","abc"),   (std::vector<int>{3}),       "pattern at end");
    CHECK_EQ(naive_search("ababab","aba"),   (std::vector<int>{0,2}),     "overlapping occurrences");
    CHECK_EQ(naive_search("a",     "a"),     (std::vector<int>{0}),       "single char text");
    CHECK_EQ(naive_search("abcde", ""),      (std::vector<int>{}),        "empty pattern");
    return true;
}

// ── Group 2: SuffixTree single-doc (vs naive oracle) ─────────────────────────

static bool test_ukkonen() {
    std::cout << "\n=== GROUP 2: SuffixTree single-doc ===\n";

    auto check = [](const std::string& text, const std::string& pattern, const char* label) -> bool {
        SuffixTree t;
        t.build(text);
        auto expected = sorted(naive_search(text, pattern));
        auto actual   = sorted(t.find_occurrences(pattern));
        if (expected != actual) {
            std::cerr << "[FAIL] " << label << " | text=\"" << text
                      << "\" pat=\"" << pattern << "\"\n";
            std::cerr << "  expected: "; for (int x:expected) std::cerr<<x<<" "; std::cerr<<"\n";
            std::cerr << "  got:      "; for (int x:actual) std::cerr<<x<<" "; std::cerr<<"\n";
            return false;
        }
        std::cout << "[OK]   " << label << "\n";
        return true;
    };

    bool ok = true;
    ok &= check("mississippi", "issi",   "mississippi/issi");
    ok &= check("banana",      "ana",    "banana/ana");
    ok &= check("abcabxabcd",  "ab",     "abcabxabcd/ab");
    ok &= check("aaaa",        "aa",     "aaaa/aa");
    ok &= check("aaaa",        "a",      "aaaa/a");
    ok &= check("abcde",       "xyz",    "abcde/xyz absent");
    ok &= check("abc",         "abc",    "abc/abc full match");
    ok &= check("a",           "a",      "single char");
    ok &= check("ababab",      "aba",    "ababab/aba overlapping");
    ok &= check("abcabxabcd",  "abcd",   "abcabxabcd/abcd");
    ok &= check("abcde",       "",       "empty pattern");

    // Terminal character must not appear in results
    {
        SuffixTree t;
        t.build("abc");
        auto res = t.find_occurrences("\0");  // searching for the terminal
        if (!res.empty()) {
            std::cerr << "[FAIL] terminal char should not be searchable\n";
            ok = false;
        } else {
            std::cout << "[OK]   terminal not searchable\n";
        }
    }

    // contains() consistency
    {
        SuffixTree t;
        t.build("mississippi");
        assert(t.contains("issi") == true);
        assert(t.contains("xyz")  == false);
        assert(t.count_occurrences("issi") == 2);
        std::cout << "[OK]   contains/count_occurrences consistency\n";
    }

    return ok;
}

// ── Group 3: CorpusTree (to be expanded in a later task) ─────────────────────

static bool test_corpus() {
    std::cout << "\n=== GROUP 3: CorpusTree (stub) ===\n";
    // Placeholder — will be filled after CorpusTree implementation
    std::cout << "[SKIP] corpus tests (not yet implemented)\n";
    return true;
}

// ── Group 4: MatchingStatistics (stub) ───────────────────────────────────────

static bool test_matching_stats() {
    std::cout << "\n=== GROUP 4: MatchingStatistics (stub) ===\n";
    std::cout << "[SKIP] matching stats tests (not yet implemented)\n";
    return true;
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    bool all_ok = true;
    all_ok &= test_naive();
    all_ok &= test_ukkonen();
    all_ok &= test_corpus();
    all_ok &= test_matching_stats();

    std::cout << "\n" << (all_ok ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    return all_ok ? 0 : 1;
}
