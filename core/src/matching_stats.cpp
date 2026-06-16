#include "matching_stats.hpp"
#include "suffix_tree.hpp"

// Simple O(n*m) matching statistics: for each position i in suspect, restart
// from the root of the corpus tree and walk down as far as possible.
// No Chang-Lawler optimization — sufficient for typical academic documents.
std::vector<MatchPos> matching_statistics(
    const std::string& suspect,
    const std::string& corpus_concat,
    const std::vector<std::pair<int,std::string>>& doc_offsets)
{
    // Stub — implemented in a later task after CorpusTree tests pass.
    (void)corpus_concat; (void)doc_offsets;
    return std::vector<MatchPos>(suspect.size(), {0, ""});
}
