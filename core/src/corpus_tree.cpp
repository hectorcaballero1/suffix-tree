#include "corpus_tree.hpp"
#include "suffix_tree.hpp"
#include <algorithm>
#include <stdexcept>

static constexpr int MAX_CORPUS_DOCS = 254; // separators \x01..\xFE

struct CorpusTree::Impl {
    SuffixTree tree;
    std::string concat;                            // concatenated normalized text
    std::vector<std::pair<int,std::string>> offsets; // (start_in_concat, source_name)
};

CorpusTree::~CorpusTree() { delete impl_; }

void CorpusTree::build(const std::vector<std::pair<std::string,std::string>>& docs) {
    if (docs.size() > static_cast<size_t>(MAX_CORPUS_DOCS))
        throw std::runtime_error("Too many corpus documents (max 254)");

    delete impl_;
    impl_ = new Impl();

    char sep = '\x01';
    for (const auto& [text, name] : docs) {
        impl_->offsets.push_back({static_cast<int>(impl_->concat.size()), name});
        impl_->concat += text;
        impl_->concat += sep++;
    }
    impl_->tree.build(impl_->concat);
}

std::string source_for_pos(const std::vector<std::pair<int,std::string>>& offsets, int pos) {
    // Binary search: find the last offset entry whose start <= pos
    int lo = 0, hi = static_cast<int>(offsets.size()) - 1, res = 0;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (offsets[mid].first <= pos) { res = mid; lo = mid + 1; }
        else hi = mid - 1;
    }
    return offsets[res].second;
}

bool CorpusTree::contains(const std::string& pattern) const {
    if (!impl_) return false;
    return impl_->tree.contains(pattern);
}

std::vector<Occurrence> CorpusTree::find_occurrences(const std::string& pattern) const {
    std::vector<Occurrence> result;
    if (!impl_) return result;
    for (int pos : impl_->tree.find_occurrences(pattern)) {
        // Skip occurrences that start inside a separator byte
        char c = impl_->concat[pos];
        if (c >= '\x01' && c <= '\xFE') continue;
        result.push_back({pos, source_for_pos(impl_->offsets, pos)});
    }
    return result;
}
