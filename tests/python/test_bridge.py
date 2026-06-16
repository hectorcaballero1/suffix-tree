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


# ── CorpusTree ────────────────────────────────────────────────────────────────

def test_corpus_shared_phrase():
    ct = stc.CorpusTree()
    ct.build([("hello world this", "doc1"), ("world this is", "doc2")])
    sources = {o.source for o in ct.find_occurrences("world")}
    assert "doc1" in sources and "doc2" in sources

def test_corpus_unique_phrase():
    ct = stc.CorpusTree()
    ct.build([("hello world", "doc1"), ("foo bar baz", "doc2")])
    occ = ct.find_occurrences("hello")
    assert len(occ) == 1 and occ[0].source == "doc1"

def test_corpus_absent_pattern():
    ct = stc.CorpusTree()
    ct.build([("hello world", "doc1"), ("foo bar baz", "doc2")])
    assert ct.find_occurrences("xyz") == []


# ── MatchingStatistics ────────────────────────────────────────────────────────

def test_matching_stats_copied_fragment():
    corpus = "abcdefghijklmnopqrstuvwxyz"
    ct = stc.CorpusTree()
    ct.build([(corpus, "source")])
    suspect = corpus[:20] + "11111111111"
    results = ct.matching_statistics(suspect, 10)
    assert len(results) > 0
    assert all(r.length >= 10 for r in results)
    assert all(r.source == "source" for r in results)
    assert all(r.pos < 11 for r in results)  # digit tail must not be flagged

def test_matching_stats_no_match():
    ct = stc.CorpusTree()
    ct.build([("hello world foo bar", "doc1")])
    assert ct.matching_statistics("xyzxyzxyz", 5) == []

def test_matching_stats_exact_copy():
    corpus = "abcdefghijklmnopqrst"
    ct = stc.CorpusTree()
    ct.build([(corpus, "ref")])
    results = ct.matching_statistics(corpus, 10)
    pos0 = [r for r in results if r.pos == 0]
    assert pos0 and pos0[0].length >= 10 and pos0[0].source == "ref"
