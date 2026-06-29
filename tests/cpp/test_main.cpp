#include <cassert>
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "naive_search.hpp"
#include "suffix_tree.hpp"
#include "corpus_tree.hpp"

// Helpers
static std::vector<int> sorted(std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}

static bool check_eq(std::vector<int> a, std::vector<int> b, const std::string& label) {
    a = sorted(a); b = sorted(b);
    if (a != b) {
        std::cerr << "[FAIL] " << label << "\n";
        std::cerr << "  expected: "; for (int x : b) std::cerr << x << " "; std::cerr << "\n";
        std::cerr << "  got:      "; for (int x : a) std::cerr << x << " "; std::cerr << "\n";
        return false;
    }
    std::cout << "[OK]   " << label << "\n";
    return true;
}

// Group 1: NaiveSearch

static bool test_naive() {
    std::cout << "\nGROUP 1: NaiveSearch\n";

    bool ok = true;
    ok &= check_eq(naive_search("abcde", "bc"),    {1},       "single occurrence");
    ok &= check_eq(naive_search("abcde", "xyz"),   {},        "pattern absent");
    ok &= check_eq(naive_search("aaaa",  "a"),     {0,1,2,3}, "all same - single char");
    ok &= check_eq(naive_search("aaaa",  "aa"),    {0,1,2},   "all same - overlapping");
    ok &= check_eq(naive_search("abcde", "abcde"), {0},       "pattern equals text");
    ok &= check_eq(naive_search("xyzabc","abc"),   {3},       "pattern at end");
    ok &= check_eq(naive_search("ababab","aba"),   {0,2},     "overlapping occurrences");
    ok &= check_eq(naive_search("a",     "a"),     {0},       "single char text");
    ok &= check_eq(naive_search("abcde", ""),      {},        "empty pattern");
    return ok;
}

// Group 2: SuffixTree single-doc
static bool test_ukkonen() {
    std::cout << "\nGROUP 2: SuffixTree single-doc\n";

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

// Group 3: CorpusTree
static bool test_corpus() {
    std::cout << "\nGROUP 3: CorpusTree\n";
    bool ok = true;

    // Shared phrase: "world" appears in both docs
    {
        CorpusTree ct;
        ct.build({{"hello world this", "doc1"}, {"world this is", "doc2"}});
        auto occ = ct.find_occurrences("world");
        std::set<std::string> sources;
        for (auto& o : occ) sources.insert(o.source);
        if (sources.count("doc1") && sources.count("doc2")) {
            std::cout << "[OK]   shared phrase found in both docs\n";
        } else {
            std::cerr << "[FAIL] shared phrase should appear in both docs\n";
            ok = false;
        }
    }

    // Unique phrase: "hello" only in doc1
    {
        CorpusTree ct;
        ct.build({{"hello world", "doc1"}, {"foo bar baz", "doc2"}});
        auto occ = ct.find_occurrences("hello");
        if (occ.size() == 1 && occ[0].source == "doc1") {
            std::cout << "[OK]   unique phrase attributed to correct doc\n";
        } else {
            std::cerr << "[FAIL] unique phrase attribution\n";
            ok = false;
        }
    }

    // Pattern absent from all docs
    {
        CorpusTree ct;
        ct.build({{"hello world", "doc1"}, {"foo bar baz", "doc2"}});
        auto occ = ct.find_occurrences("xyz");
        if (occ.empty()) {
            std::cout << "[OK]   absent pattern gives empty result\n";
        } else {
            std::cerr << "[FAIL] expected empty for absent pattern\n";
            ok = false;
        }
    }

    // contains() consistency
    {
        CorpusTree ct;
        ct.build({{"abcdef", "d1"}, {"ghijkl", "d2"}});
        assert(ct.contains("abc") == true);
        assert(ct.contains("ghi") == true);
        assert(ct.contains("xyz") == false);
        std::cout << "[OK]   contains() consistency\n";
    }

    return ok;
}

// Group 4: MatchingStatistics
static bool test_matching_stats() {
    std::cout << "\n=== GROUP 4: MatchingStatistics ===\n";
    bool ok = true;

    // Copied fragment detected
    {
        // corpus: 26-char alphabet; suspect: first 20 chars of corpus + 11 digits
        std::string corpus = "abcdefghijklmnopqrstuvwxyz";
        CorpusTree ct;
        ct.build({{corpus, "source"}});
        std::string suspect = corpus.substr(0, 20) + "11111111111";
        auto res = ct.matching_statistics(suspect, 10);
        bool found = !res.empty() && res[0].length >= 10 && res[0].source == "source";
        if (found) {
            std::cout << "[OK]   copied fragment detected with correct source\n";
        } else {
            std::cerr << "[FAIL] copied fragment not detected\n";
            ok = false;
        }
        // No match should appear in the digit-only suffix (pos >= 11 => len < 10)
        bool false_positive = false;
        for (auto& r : res)
            if (r.pos >= 11) { false_positive = true; break; }
        if (!false_positive) {
            std::cout << "[OK]   original suffix not flagged as plagiarism\n";
        } else {
            std::cerr << "[FAIL] digits incorrectly flagged\n";
            ok = false;
        }
    }

    // Nothing from corpus -> empty
    {
        CorpusTree ct;
        ct.build({{"hello world foo bar", "doc1"}});
        auto res = ct.matching_statistics("xyzxyzxyz", 5);
        if (res.empty()) {
            std::cout << "[OK]   no match gives empty result\n";
        } else {
            std::cerr << "[FAIL] expected empty for unrelated suspect\n";
            ok = false;
        }
    }

    // Exact copy of corpus -> entire suspect flagged at position 0
    {
        std::string corpus = "abcdefghijklmnopqrst"; // 20 chars
        CorpusTree ct;
        ct.build({{corpus, "ref"}});
        auto res = ct.matching_statistics(corpus, 10);
        bool pos0_ok = false;
        for (auto& r : res)
            if (r.pos == 0 && r.length >= 10 && r.source == "ref") { pos0_ok = true; break; }
        if (pos0_ok) {
            std::cout << "[OK]   exact copy flagged at position 0\n";
        } else {
            std::cerr << "[FAIL] exact copy not flagged at position 0\n";
            ok = false;
        }
    }

    return ok;
}

// Main
int main() {
    bool all_ok = true;
    all_ok &= test_naive();
    all_ok &= test_ukkonen();
    all_ok &= test_corpus();
    all_ok &= test_matching_stats();

    std::cout << "\n" << (all_ok ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    return all_ok ? 0 : 1;
}
