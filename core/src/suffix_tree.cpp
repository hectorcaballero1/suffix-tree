#include "suffix_tree.hpp"

SuffixTree::~SuffixTree() {
    if (root_) destroy(root_);
}

void SuffixTree::destroy(Node* n) {
    for (auto& [ch, child] : n->children)
        destroy(child);
    // Internal nodes own their end pointer; leaves share global_end_ (do not delete)
    if (n->suffix_index == -1)
        delete n->end;
    delete n;
}

Node* SuffixTree::new_node(int start, int* end) {
    return new Node(start, end);
}

// Build (Ukkonen)
void SuffixTree::build(const std::string& text) {
    text_ = text + '\0'; // terminal character outside the normalized alphabet
    global_end_ = -1;

    if (root_) destroy(root_);
    // Root: dummy edge [-1, -1], leaf logic skips edge_length for root
    root_ = new_node(-1, new int(-1));
    root_->suffix_link = root_;

    active_node_ = root_;
    active_edge_ = 0;
    active_len_ = 0;
    remaining_ = 0;
    last_new_internal_ = nullptr;

    for (int i = 0; i < static_cast<int>(text_.size()); ++i)
        extend(i);
}

void SuffixTree::extend(int phase) {
    ++global_end_;
    ++remaining_;
    last_new_internal_ = nullptr;

    while (remaining_ > 0) {
        if (active_len_ == 0)
            active_edge_ = phase; // start of the new suffix we're trying to insert

        char ae_char = text_[active_edge_];
        auto it = active_node_->children.find(ae_char);

        if (it == active_node_->children.end()) {
            // Rule 2: no edge starts with this char — create a new leaf.
            // Suffix index: current phase minus how many suffixes are still pending.
            Node* leaf = new_node(phase, &global_end_);
            leaf->suffix_index = phase - remaining_ + 1;
            active_node_->children[ae_char] = leaf;
            if (last_new_internal_) {
                last_new_internal_->suffix_link = active_node_;
                last_new_internal_ = nullptr;
            }
        } else {
            Node* nxt = it->second;
            // Walk down if active_len_ >= edge length (canonicalize active point)
            int edge_len = *nxt->end - nxt->start + 1;
            if (active_len_ >= edge_len) {
                active_edge_ += edge_len;
                active_len_ -= edge_len;
                active_node_  = nxt;
                continue; // retry with new active point
            }

            // Rule 3: next char on the active edge already matches
            if (text_[nxt->start + active_len_] == text_[phase]) {
                ++active_len_;
                if (last_new_internal_)
                    last_new_internal_->suffix_link = active_node_;
                break; // show stopper — all remaining suffixes extend implicitly
            }

            // Rule 2: split the edge
            int* split_end = new int(nxt->start + active_len_ - 1);
            Node* split = new_node(nxt->start, split_end);
            active_node_->children[ae_char] = split;

            // New leaf for the current phase character
            Node* new_leaf = new_node(phase, &global_end_);
            new_leaf->suffix_index = phase - remaining_ + 1;
            split->children[text_[phase]] = new_leaf;

            // Existing node continues from the split point
            nxt->start += active_len_;
            split->children[text_[nxt->start]] = nxt;

            if (last_new_internal_)
                last_new_internal_->suffix_link = split;
            last_new_internal_ = split;
        }

        --remaining_;
        if (active_node_ == root_ && active_len_ > 0) {
            --active_len_;
            active_edge_ = phase - remaining_ + 1;
        } else if (active_node_->suffix_link && active_node_ != root_) {
            active_node_ = active_node_->suffix_link;
        } else {
            active_node_ = root_;
        }
    }
}

// DFS helpers
void SuffixTree::collect_leaves(Node* n, std::vector<int>& out) const {
    if (n->children.empty()) {
        if (n->suffix_index >= 0)
            out.push_back(n->suffix_index);
        return;
    }
    for (auto& [ch, child] : n->children)
        collect_leaves(child, out);
}

// Search
bool SuffixTree::contains(const std::string& pattern) const {
    if (!root_ || pattern.empty()) return false;
    int dummy = 0;
    return find_node(pattern, dummy) != nullptr;
}

int SuffixTree::count_occurrences(const std::string& pattern) const {
    return static_cast<int>(find_occurrences(pattern).size());
}

std::vector<int> SuffixTree::find_occurrences(const std::string& pattern) const {
    std::vector<int> result;
    if (!root_ || pattern.empty()) return result;
    int edge_offset = 0;
    Node* n = find_node(pattern, edge_offset);
    if (!n) return result;
    collect_leaves(n, result);
    return result;
}

std::vector<std::string> SuffixTree::get_tree_path(const std::string& pattern) const {
    std::vector<std::string> path;
    if (!root_ || pattern.empty()) return path;
    Node* cur = root_;
    int pi = 0;
    const int m = static_cast<int>(pattern.size());
    while (pi < m) {
        auto it = cur->children.find(pattern[pi]);
        if (it == cur->children.end()) return {}; // no match
        Node* child = it->second;
        int edge_len = *child->end - child->start + 1;
        int match = 0;
        while (match < edge_len && pi < m) {
            if (text_[child->start + match] != pattern[pi]) return {}; // mismatch
            ++match; ++pi;
        }
        path.push_back(text_.substr(child->start, match));
        if (pi < m) cur = child;
    }
    return path;
}

std::pair<int,int> SuffixTree::longest_match(const std::string& query, int start) const {
    if (!root_ || start >= (int)query.size()) return {0, -1};

    Node* cur = root_;
    int matched = 0;

    while (true) {
        int qi = start + matched;
        if (qi >= (int)query.size()) break;

        auto it = cur->children.find(query[qi]);
        if (it == cur->children.end()) break;

        Node* child = it->second;
        int edge_len = *child->end - child->start + 1;
        int ei = 0;

        while (ei < edge_len && qi + ei < (int)query.size()) {
            if (text_[child->start + ei] != query[qi + ei]) {
                matched += ei;
                std::vector<int> tmp;
                collect_leaves(child, tmp);
                return {matched, tmp.empty() ? -1 : tmp[0]};
            }
            ei++;
        }

        matched += ei;

        if (qi + ei >= (int)query.size()) {
            // query exhausted mid-edge or exactly at edge end
            std::vector<int> tmp;
            collect_leaves(child, tmp);
            return {matched, tmp.empty() ? -1 : tmp[0]};
        }

        cur = child; // consumed full edge, descend
    }

    if (matched > 0) {
        std::vector<int> tmp;
        collect_leaves(cur, tmp);
        return {matched, tmp.empty() ? -1 : tmp[0]};
    }
    return {0, -1};
}

Node* SuffixTree::find_node(const std::string& pattern, int& edge_offset) const {
    Node* cur = root_;
    int pi = 0;
    const int m = static_cast<int>(pattern.size());
    while (pi < m) {
        auto it = cur->children.find(pattern[pi]);
        if (it == cur->children.end()) return nullptr;
        Node* child = it->second;
        int edge_len = *child->end - child->start + 1;
        int match = 0;
        while (match < edge_len && pi < m) {
            if (text_[child->start + match] != pattern[pi]) return nullptr;
            ++match; ++pi;
        }
        if (pi == m) {
            edge_offset = match;
            return child;
        }
        cur = child;
    }
    edge_offset = 0;
    return cur;
}
