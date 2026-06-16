"""Python-level tests for the pybind11 bridge. Mirrors a subset of the C++ tests."""
import sys
import os
import pytest

# The .so is placed in backend/ by CMake
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../backend"))
import suffix_tree_core as stc  # noqa: E402


def sorted_occ(tree, pattern):
    return sorted(tree.find_occurrences(pattern))


def naive(text, pattern):
    return sorted(stc.naive_search(text, pattern))


def test_naive_oracle():
    assert stc.naive_search("abcde", "bc") == [1]
    assert stc.naive_search("abcde", "xyz") == []
    assert sorted(stc.naive_search("aaaa", "a")) == [0, 1, 2, 3]


def test_suffix_tree_mississippi():
    t = stc.SuffixTree()
    t.build("mississippi")
    assert sorted_occ(t, "issi") == sorted(naive("mississippi", "issi"))


def test_suffix_tree_banana():
    t = stc.SuffixTree()
    t.build("banana")
    assert sorted_occ(t, "ana") == sorted(naive("banana", "ana"))


def test_suffix_tree_no_match():
    t = stc.SuffixTree()
    t.build("abcde")
    assert t.find_occurrences("xyz") == []
    assert t.contains("xyz") is False


def test_suffix_tree_contains():
    t = stc.SuffixTree()
    t.build("mississippi")
    assert t.contains("issi") is True
    assert t.count_occurrences("issi") == 2


def test_suffix_tree_empty_pattern():
    t = stc.SuffixTree()
    t.build("hello")
    assert t.find_occurrences("") == []
