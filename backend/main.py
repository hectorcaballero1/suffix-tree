import os
import time
import shutil
from pathlib import Path
from contextlib import asynccontextmanager

from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from text_utils import (
    extract_text, normalize_text, ensure_dirs,
    SEARCH_DIR, CORPUS_DIR, BENCHMARK_DIR,
)
from corpus_manager import doc_state, corpus_state, MAX_CORPUS_DOCS
import suffix_tree_core as stc


@asynccontextmanager
async def lifespan(app: FastAPI):
    ensure_dirs()
    yield


app = FastAPI(title="Suffix Tree Search & Plagiarism Detector", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# ── Schemas ────────────────────────────────────────────────────────────────────

class UploadResponse(BaseModel):
    saved: list[str]
    total_chars: int

class BuildResponse(BaseModel):
    sources: list[str]
    total_chars: int
    build_time_ms: float

class DocUploadResponse(BaseModel):
    filename: str
    char_count: int

class DocLoadResponse(BaseModel):
    filename: str
    text_original: str
    char_count: int
    build_time_ms: float

class SearchRequest(BaseModel):
    pattern: str

class OccurrenceItem(BaseModel):
    position: int
    original_position: int

class SearchResponse(BaseModel):
    occurrences: list[int]
    original_occurrences: list[int]
    count: int
    tree_path: list[str]
    search_time_ms: float

class DetectRequest(BaseModel):
    min_match_length: int = 40

class SpanItem(BaseModel):
    start: int
    end: int
    source: str
    length: int
    original_start: int
    original_end: int

class DetectResponse(BaseModel):
    spans: list[SpanItem]
    global_pct: float
    by_source: dict[str, float]

class BenchmarkRequest(BaseModel):
    pattern: str = "the"

class BenchmarkResult(BaseModel):
    size: int
    file: str
    suffix_tree_build_ms: float
    suffix_tree_search_ms: float
    naive_search_ms: float
    occurrences: int

class BenchmarkResponse(BaseModel):
    results: list[BenchmarkResult]

class ErrorResponse(BaseModel):
    error: str


# ── Corpus endpoints ───────────────────────────────────────────────────────────

@app.post("/corpus/upload")
async def corpus_upload(files: list[UploadFile] = File(...)):
    ensure_dirs()
    saved = []
    total_chars = 0
    for f in files:
        content = await f.read()
        fpath = CORPUS_DIR / f.filename
        with open(fpath, "wb") as out:
            out.write(content)
        saved.append(f.filename)
        try:
            text = extract_text(fpath)
            total_chars += len(text)
        except Exception:
            pass
    return UploadResponse(saved=saved, total_chars=total_chars)


@app.post("/corpus/build")
async def corpus_build():
    ensure_dirs()
    try:
        corpus_state.build_from_dir(str(CORPUS_DIR))
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    return BuildResponse(
        sources=corpus_state.sources,
        total_chars=corpus_state.total_chars,
        build_time_ms=corpus_state.build_time_ms,
    )


# ── Document endpoints ─────────────────────────────────────────────────────────

@app.post("/document/upload")
async def document_upload(file: UploadFile = File(...)):
    ensure_dirs()
    for old in SEARCH_DIR.iterdir():
        if old.is_file() and not old.name.startswith("."):
            old.unlink()
    content = await file.read()
    fpath = SEARCH_DIR / file.filename
    with open(fpath, "wb") as out:
        out.write(content)
    text = extract_text(fpath)
    return DocUploadResponse(filename=file.filename, char_count=len(text))


@app.post("/document/load")
async def document_load():
    ensure_dirs()
    files = [f for f in SEARCH_DIR.iterdir() if f.is_file() and not f.name.startswith(".")]
    if not files:
        raise HTTPException(status_code=400, detail="search_doc_missing")
    if len(files) > 1:
        raise HTTPException(status_code=400, detail="search_doc_multiple")
    fpath = files[0]
    text = extract_text(fpath)
    doc_state.load(text, fpath.name)
    return DocLoadResponse(
        filename=doc_state.filename,
        text_original=doc_state.original_text,
        char_count=len(doc_state.normalized_text),
        build_time_ms=doc_state.build_time_ms,
    )


@app.post("/document/search")
async def document_search(req: SearchRequest):
    if not doc_state.normalized_text:
        raise HTTPException(status_code=400, detail="no_document")
    if not req.pattern.strip():
        raise HTTPException(status_code=400, detail="empty_pattern")
    pattern = normalize_text(req.pattern.strip())
    t0 = time.perf_counter()
    occ = doc_state.tree.find_occurrences(pattern)
    search_time = (time.perf_counter() - t0) * 1000
    orig_occ = [doc_state.translate_position(p) for p in occ]
    tree_path = doc_state.tree.get_tree_path(pattern)
    return SearchResponse(
        occurrences=occ,
        original_occurrences=orig_occ,
        count=len(occ),
        tree_path=tree_path,
        search_time_ms=search_time,
    )


# ── Plagiarism detection ───────────────────────────────────────────────────────

def merge_spans(results, doc_norm_text, min_gap=5):
    if not results:
        return []
    by_source: dict[str, list] = {}
    for r in results:
        by_source.setdefault(r.source, []).append((r.pos, r.pos + r.length))
    merged = []
    for src, intervals in by_source.items():
        intervals.sort()
        cur_start, cur_end = intervals[0]
        for s, e in intervals[1:]:
            if s - cur_end <= min_gap:
                cur_end = max(cur_end, e)
            else:
                merged.append((cur_start, cur_end, src))
                cur_start, cur_end = s, e
        merged.append((cur_start, cur_end, src))
    return merged


@app.post("/detect")
async def detect(req: DetectRequest):
    if not corpus_state.is_built():
        raise HTTPException(status_code=400, detail="no_corpus")
    if not doc_state.normalized_text:
        raise HTTPException(status_code=400, detail="no_document")

    t0 = time.perf_counter()
    results = corpus_state.tree.matching_statistics(
        doc_state.normalized_text, req.min_match_length
    )
    search_time = (time.perf_counter() - t0) * 1000

    merged = merge_spans(results, doc_state.normalized_text)
    norm_len = len(doc_state.normalized_text)
    total_matched = 0
    by_source: dict[str, int] = {}
    span_items = []
    for start, end, src in merged:
        span_len = end - start
        total_matched += span_len
        by_source[src] = by_source.get(src, 0) + span_len
        orig_start = doc_state.translate_position(start)
        orig_end = doc_state.translate_position(min(end, norm_len - 1)) + 1
        span_items.append(SpanItem(
            start=start, end=end, source=src, length=span_len,
            original_start=orig_start, original_end=orig_end,
        ))

    global_pct = (total_matched / norm_len * 100) if norm_len > 0 else 0.0
    by_source_pct = {
        src: (count / norm_len * 100) for src, count in by_source.items()
    }

    return DetectResponse(
        spans=span_items,
        global_pct=round(global_pct, 2),
        by_source={k: round(v, 2) for k, v in by_source_pct.items()},
    )


# ── Benchmark ──────────────────────────────────────────────────────────────────

@app.post("/benchmark")
async def benchmark(req: BenchmarkRequest):
    ensure_dirs()
    benchmark_files = sorted(
        [f for f in BENCHMARK_DIR.iterdir() if f.is_file() and not f.name.startswith(".")],
        key=lambda p: p.stat().st_size,
    )
    if not benchmark_files:
        raise HTTPException(status_code=400, detail="benchmark_file_missing")

    pattern = req.pattern
    results = []

    for fpath in benchmark_files:
        text = extract_text(fpath)
        size = len(text)
        fname = fpath.name

        t0 = time.perf_counter()
        tree = stc.SuffixTree()
        tree.build(text)
        build_ms = (time.perf_counter() - t0) * 1000

        t0 = time.perf_counter()
        occ = tree.find_occurrences(pattern)
        search_ms = (time.perf_counter() - t0) * 1000

        t0 = time.perf_counter()
        naive_occ = stc.naive_search(text, pattern)
        naive_ms = (time.perf_counter() - t0) * 1000

        results.append(BenchmarkResult(
            size=size,
            file=fname,
            suffix_tree_build_ms=round(build_ms, 4),
            suffix_tree_search_ms=round(search_ms, 6),
            naive_search_ms=round(naive_ms, 4),
            occurrences=len(occ),
        ))

    return BenchmarkResponse(results=results)


# ── Health ─────────────────────────────────────────────────────────────────────

@app.get("/health")
async def health():
    return {"status": "ok"}
