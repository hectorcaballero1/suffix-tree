import unicodedata
import os
from pathlib import Path


TESTING = os.getenv("SUFFIX_TREE_TESTING", "0") == "1"
if TESTING:
    DATA_DIR = Path(__file__).parent / "test_data"
else:
    DATA_DIR = Path(__file__).parent / "data"

SEARCH_DIR = DATA_DIR / "search_doc"
CORPUS_DIR = DATA_DIR / "corpus"
BENCHMARK_DIR = DATA_DIR / "benchmark"


def normalize_text(text: str) -> str:
    text = text.lower()
    text = text.replace("\u00f1", "~").replace("\u00d1", "~")
    nfkd = unicodedata.normalize("NFKD", text)
    text = "".join(c for c in nfkd if not unicodedata.combining(c))
    text = "".join(c for c in text if c.isascii() or c in ("\n", "\t"))
    return text


def offset_map_from_original(original: str, normalized: str) -> list[int]:
    mapping = []
    ni = 0
    for oi, ch in enumerate(original):
        if ni < len(normalized):
            low = ch.lower()
            nf_char = low
            if low in ("\u00f1", "\u00d1"):
                nf_char = "~"
            else:
                nfkd = unicodedata.normalize("NFKD", low)
                nf_char = "".join(c for c in nfkd if not unicodedata.combining(c))
            if len(nf_char) > 0 and nf_char.isascii():
                if ni < len(normalized) and normalized[ni] == nf_char:
                    mapping.append(oi)
                    ni += 1
    while ni < len(normalized):
        mapping.append(len(original) - 1 if original else 0)
        ni += 1
    return mapping


def extract_text_from_pdf(path: str | Path) -> str:
    import fitz
    doc = fitz.open(str(path))
    text = "".join(page.get_text() for page in doc)
    doc.close()
    return text


def extract_text_from_txt(path: str | Path) -> str:
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def extract_text(path: str | Path) -> str:
    path = Path(path)
    ext = path.suffix.lower()
    if ext == ".pdf":
        return extract_text_from_pdf(path)
    elif ext == ".txt":
        return extract_text_from_txt(path)
    else:
        raise ValueError(f"Unsupported file type: {ext}")


def generate_benchmark_files():
    files = {
        "benchmark_100k.txt": 100_000,
        "benchmark_500k.txt": 500_000,
        "benchmark_1M.txt": 1_000_000,
    }
    words = [
        "the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog",
        "algorithm", "structure", "data", "suffix", "tree", "ukkonen",
        "computer", "science", "plagiarism", "detector", "search", "engine",
        "a", "an", "is", "was", "for", "with", "on", "at", "by", "from"
    ]
    import random
    random.seed(42)
    for fname, size in files.items():
        fpath = BENCHMARK_DIR / fname
        if not fpath.exists():
            text_parts = []
            current_len = 0
            while current_len < size:
                word = random.choice(words)
                text_parts.append(word)
                current_len += len(word) + 1
            text = " ".join(text_parts)[:size]
            with open(fpath, "w", encoding="utf-8") as f:
                f.write(text)


def ensure_dirs():
    for d in [SEARCH_DIR, CORPUS_DIR, BENCHMARK_DIR]:
        d.mkdir(parents=True, exist_ok=True)
    generate_benchmark_files()

