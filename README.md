# Suffix Tree

Busqueda indexada de patrones y deteccion de plagio sobre documentos PDF/TXT. Nucleo en C++ (Ukkonen) expuesto a Python via pybind11, backend FastAPI.

## Levantar

```bash
uv run build.py                          # compila el modulo C++
cd backend && uv run uvicorn main:app    # http://localhost:8000
uv run test.py                           # tests
```

---

## Endpoints

### `POST /document/upload`
Recibe un archivo (PDF o TXT), extrae texto, normaliza y guarda en `data/search_doc/` (reemplaza cualquier archivo anterior).

Request: `multipart/form-data`, campo `file`.

Response: `{ "filename": "doc.pdf", "char_count": 8400 }`

### `POST /document/load`
Construye el `SuffixTree` en memoria sobre el archivo subido.

Response: `{ "filename", "text_original", "char_count", "build_time_ms" }`

Errores: `search_doc_missing`, `search_doc_multiple`.

### `POST /document/search`
Busca un patron en el documento cargado. Devuelve posiciones en texto original.

Request: `{ "pattern": "redes neuronales" }`

Response:
```json
{ "occurrences": [142, 891], "count": 2, "tree_path": ["redes ", "neur", "onales"], "search_time_ms": 0.08 }
```

`tree_path`: aristas recorridas en el suffix tree (para visualizacion educativa).

Errores: `no_document`, `empty_pattern`.

---

### `POST /corpus/upload`
Recibe uno o varios archivos, extrae texto y guarda en `data/corpus/`. No construye el arbol todavia.

Request: `multipart/form-data`, campo `files[]`.

Response: `{ "saved": ["ref1.txt"], "total_chars": 142300 }`

### `POST /corpus/build`
Construye el `CorpusTree` generalizado en memoria con todo lo que haya en `data/corpus/`.

Response: `{ "sources": ["ref1.txt"], "total_chars": 142300, "build_time_ms": 87.4 }`

Errores: `corpus_dir_empty`, `corpus_too_large` (> 2M chars), `too_many_documents` (> 254).

### `POST /detect`
Corre matching statistics del documento cargado contra el corpus. Devuelve spans en texto original.

Request: `{ "min_match_length": 40 }`

Response:
```json
{ "spans": [{ "start": 142, "end": 298, "source": "ref1.txt", "length": 156 }], "global_pct": 18.4, "by_source": { "ref1.txt": 18.4 } }
```

Errores: `no_corpus`, `no_document`.

---

### `POST /benchmark/upload`
Recibe archivos, extrae texto y los guarda en `data/benchmark/`. Cualquier archivo en esa carpeta entra al benchmark automaticamente.

### `POST /benchmark`
Corre el benchmark sobre todos los archivos en `data/benchmark/`: construye un SuffixTree por archivo, mide construccion y busqueda ST vs naive.

Request: `{ "pattern": "the" }`

Response: `{ "results": [{ "file", "size", "word_count", "suffix_tree_build_ms", "suffix_tree_search_ms", "naive_search_ms", "occurrences" }] }`

---

## Decisiones tecnicas

**Ukkonen O(n):** el suffix tree se construye en O(n) tiempo y espacio. La alternativa naive (insertar cada sufijo uno a uno) seria O(n²). Ukkonen mantiene un "punto activo" y reglas de extension que permiten procesar cada caracter exactamente una vez.

**Terminal `\x00`:** se agrega al final del texto antes de construir el arbol para garantizar que todos los sufijos terminen en hojas distintas. Sin el, sufijos que son prefijos de otros podrian "perderse" dentro del arbol.

**CorpusTree con separadores `\x01`–`\xFE`:** en lugar de construir N arboles separados para el corpus, se concatenan todos los documentos con un byte separador unico por documento y se construye un solo arbol. Permite buscar un patron en todos los documentos a la vez en O(m). Limite de 254 documentos porque solo hay 254 valores de separador disponibles.

**Matching statistics:** para cada posicion `i` del texto sospechoso se baja desde la raiz del CorpusTree midiendo cuantos caracteres consecutivos aparecen en el corpus. Posiciones donde esa longitud supera `min_match_length` son candidatas a plagio. Python fusiona posiciones adyacentes del mismo source en spans y los traduce a coordenadas del texto original.

**Normalizacion:** el C++ trabaja sobre ASCII puro (sin acentos ni puntuacion). Python mantiene un `offset_map` que traduce cada posicion normalizada a su posicion en el texto original, para que el frontend pueda resaltar el texto tal como fue subido. La `ñ` se mapea a `~` antes de aplicar NFKD para evitar que "año" y "ano" sean identicos.
