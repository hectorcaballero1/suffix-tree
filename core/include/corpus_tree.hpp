#pragma once
#include <string>
#include <vector>
#include <utility>

struct Occurrence {
    int position;       // in the concatenated normalized corpus text
    std::string source; // document name
};

// Generalized suffix tree for a corpus of documents.
// Documents are concatenated with unique single-byte separators \x01, \x02, ...
// Limit: 254 documents max (\x01..\xFE). Documented here so callers can enforce it.
class CorpusTree {
public:
    CorpusTree() = default;
    ~CorpusTree();

    // docs: list of (normalized_text, source_name) pairs.
    void build(const std::vector<std::pair<std::string, std::string>>& docs);

    bool contains(const std::string& pattern) const;
    std::vector<Occurrence> find_occurrences(const std::string& pattern) const;

private:
    // The generalized tree is built on a single concatenated string using SuffixTree
    // internally; source attribution is done via doc_offsets_.
    struct Impl;
    Impl* impl_ = nullptr;
};
