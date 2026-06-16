#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct Node {
    std::unordered_map<char, Node*> children;
    Node* suffix_link = nullptr;
    int start;
    int* end;       // points to global_end for leaves; owned int for internal nodes
    int suffix_index = -1;

    Node(int start, int* end) : start(start), end(end) {}
    int edge_length() const { return *end - start + 1; }
};

class SuffixTree {
public:
    SuffixTree() = default;
    ~SuffixTree();

    // Build the suffix tree for `text` using Ukkonen's algorithm.
    // A unique terminal character '\0' is appended internally.
    void build(const std::string& text);

    bool contains(const std::string& pattern) const;
    int  count_occurrences(const std::string& pattern) const;
    std::vector<int> find_occurrences(const std::string& pattern) const;

    // Returns the sequence of edge-label substrings traversed when matching pattern.
    std::vector<std::string> get_tree_path(const std::string& pattern) const;

    // Walk from root matching query[start:] as far as possible.
    // Returns {matched_length, suffix_index_of_one_occurrence}, or {0,-1} if no match.
    std::pair<int,int> longest_match(const std::string& query, int start) const;

private:
    std::string text_;
    int global_end_ = -1;

    Node* root_       = nullptr;
    Node* active_node_= nullptr;
    int   active_edge_= 0;    // index into text_ of the first char of the active edge
    int   active_len_ = 0;
    int   remaining_  = 0;

    Node* last_new_internal_ = nullptr;

    Node* new_node(int start, int* end);
    void  extend(int phase);
    void  walk_down(Node* n);

    // Helpers for search
    Node* find_node(const std::string& pattern, int& edge_offset) const;
    void  collect_leaves(Node* n, std::vector<int>& out) const;

    void destroy(Node* n);
};
