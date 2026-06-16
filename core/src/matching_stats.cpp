#include "matching_stats.hpp"
#include "suffix_tree.hpp"

std::vector<MatchPos> matching_statistics(
    const std::string& suspect,
    const std::string& corpus_concat,
    const std::vector<std::pair<int,std::string>>& doc_offsets)
{
    SuffixTree tree;
    tree.build(corpus_concat);
    std::vector<MatchPos> result(suspect.size(), {0, ""});
    for (int i = 0; i < static_cast<int>(suspect.size()); ++i) {
        auto [len, pos] = tree.longest_match(suspect, i);
        if (len > 0 && pos >= 0) {
            int lo = 0, hi = static_cast<int>(doc_offsets.size()) - 1, res = 0;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                if (doc_offsets[mid].first <= pos) { res = mid; lo = mid + 1; }
                else hi = mid - 1;
            }
            result[i] = {len, doc_offsets[res].second};
        }
    }
    return result;
}
