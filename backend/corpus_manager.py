import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import suffix_tree_core as stc

from text_utils import normalize_text, offset_map_from_original


MAX_CORPUS_CHARS = 10_000_000  # safety cap (RAM); algorithm itself handles up to ~INT_MAX chars
MAX_CORPUS_DOCS = 128  # C++ core uses high-byte separators \x80..\xFF (128 max)


class DocState:
    def __init__(self):
        self.tree = stc.SuffixTree()
        self.original_text = ""
        self.normalized_text = ""
        self.offset_map: list[int] = []
        self.filename = ""
        self.build_time_ms = 0.0

    def load(self, text: str, filename: str):
        self.original_text = text
        self.normalized_text = normalize_text(text)
        self.offset_map = offset_map_from_original(text, self.normalized_text)
        self.filename = filename
        import time
        t0 = time.perf_counter()
        self.tree.build(self.normalized_text)
        self.build_time_ms = (time.perf_counter() - t0) * 1000

    def translate_position(self, norm_pos: int) -> int:
        if 0 <= norm_pos < len(self.offset_map):
            return self.offset_map[norm_pos]
        return norm_pos

    def clear(self):
        self.tree = stc.SuffixTree()
        self.original_text = ""
        self.normalized_text = ""
        self.offset_map = []
        self.filename = ""
        self.build_time_ms = 0.0


class CorpusState:
    def __init__(self):
        self.tree = stc.CorpusTree()
        self.sources: list[str] = []
        self.total_chars = 0
        self.build_time_ms = 0.0
        self._built = False

    def build_from_dir(self, corpus_dir):
        import time, os
        docs = []
        self.sources = []
        self.total_chars = 0

        for fname in sorted(os.listdir(corpus_dir)):
            if fname.startswith("."):
                continue
            fpath = os.path.join(corpus_dir, fname)
            if not os.path.isfile(fpath):
                continue
            from text_utils import extract_text
            raw = extract_text(fpath)
            norm = normalize_text(raw)
            docs.append((norm, fname))
            self.sources.append(fname)
            self.total_chars += len(norm)

        if not docs:
            raise ValueError("corpus_dir_empty")
        if len(docs) > MAX_CORPUS_DOCS:
            raise ValueError("too_many_documents")
        if self.total_chars > MAX_CORPUS_CHARS:
            raise ValueError("corpus_too_large")

        t0 = time.perf_counter()
        self.tree.build(docs)
        self.build_time_ms = (time.perf_counter() - t0) * 1000
        self._built = True

    def is_built(self) -> bool:
        return self._built

    def clear(self):
        self.tree = stc.CorpusTree()
        self.sources = []
        self.total_chars = 0
        self.build_time_ms = 0.0
        self._built = False


doc_state = DocState()
corpus_state = CorpusState()
