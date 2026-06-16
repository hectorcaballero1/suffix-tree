#pragma once
#include <string>
#include <vector>

struct MatchPos {
    int length;         // length of longest match starting at this position
    std::string source; // source document of the match (empty if length == 0)
};

// Simple matching statistics: for each position i in `suspect`, restart from
// the root of the corpus tree and measure the longest prefix of suspect[i:]
// that appears in the corpus. O(n * m) in the worst case but simple and correct.
std::vector<MatchPos> matching_statistics(
    const std::string& suspect_normalized,
    const std::string& corpus_concatenated,
    const std::vector<std::pair<int,std::string>>& doc_offsets
);
