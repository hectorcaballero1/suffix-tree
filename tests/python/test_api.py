import os
import shutil
import pytest
from pathlib import Path

# Enable testing sandbox before importing any backend code
os.environ["SUFFIX_TREE_TESTING"] = "1"

import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../backend"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from fastapi.testclient import TestClient
from main import app
from text_utils import DATA_DIR, ensure_dirs


@pytest.fixture(autouse=True)
def setup_and_teardown():
    # Cleanup test_data directory if it exists
    if DATA_DIR.exists():
        shutil.rmtree(DATA_DIR)
    ensure_dirs()
    yield
    # Cleanup after test runs
    if DATA_DIR.exists():
        shutil.rmtree(DATA_DIR)


def test_health():
    client = TestClient(app)
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_complete_flow():
    client = TestClient(app)

    # 1. Upload corpus files
    files = [
        ("files", ("ref1.txt", b"The quick brown fox jumps over the lazy dog")),
        ("files", ("ref2.txt", b"Artificial intelligence is transforming computer science")),
    ]
    response = client.post("/corpus/upload", files=files)
    assert response.status_code == 200
    data = response.json()
    assert "ref1.txt" in data["saved"]
    assert "ref2.txt" in data["saved"]
    assert data["total_chars"] > 0

    # 2. Build corpus tree
    response = client.post("/corpus/build")
    assert response.status_code == 200
    data = response.json()
    assert "ref1.txt" in data["sources"]
    assert "ref2.txt" in data["sources"]
    assert data["total_chars"] > 0
    assert "build_time_ms" in data

    # 3. Upload a suspect document
    suspect_content = b"The quick brown fox jumps over the lazy dog! Artificial intelligence is cool."
    response = client.post(
        "/document/upload",
        files={"file": ("suspect.txt", suspect_content)}
    )
    assert response.status_code == 200
    data = response.json()
    assert data["filename"] == "suspect.txt"
    assert data["char_count"] == len(suspect_content)

    # 4. Load the document into memory
    response = client.post("/document/load")
    assert response.status_code == 200
    data = response.json()
    assert data["filename"] == "suspect.txt"
    assert "text_original" in data
    assert "build_time_ms" in data

    # 5. Search pattern in document (Ctrl+F)
    response = client.post("/document/search", json={"pattern": "brown fox"})
    assert response.status_code == 200
    data = response.json()
    assert data["count"] == 1
    assert len(data["occurrences"]) == 1
    assert len(data["original_occurrences"]) == 1
    assert data["original_occurrences"][0] == suspect_content.find(b"brown fox")
    assert "tree_path" in data
    assert len(data["tree_path"]) > 0

    # 6. Detect plagiarism against corpus
    response = client.post("/detect", json={"min_match_length": 10})
    assert response.status_code == 200
    data = response.json()
    assert len(data["spans"]) > 0
    assert data["global_pct"] > 0.0
    assert "ref1.txt" in data["by_source"]
    
    # Check span structure
    span = data["spans"][0]
    assert "start" in span
    assert "end" in span
    assert "source" in span
    assert "length" in span
    assert "original_start" in span
    assert "original_end" in span

    # 7. Run Benchmark
    response = client.post("/benchmark", json={"pattern": "fox"})
    assert response.status_code == 200
    data = response.json()
    assert "results" in data
    assert len(data["results"]) == 3 # 100k, 500k, 1M files auto-generated
    for result in data["results"]:
        assert "size" in result
        assert "file" in result
        assert "suffix_tree_build_ms" in result
        assert "suffix_tree_search_ms" in result
        assert "naive_search_ms" in result
        assert "occurrences" in result
