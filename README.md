# Suffix Tree 

Proyecto de Algoritmos y Estructuras de datos.

- **Ctrl+F indexado:** busqueda de patrones O(m) sobre un documento via suffix tree.
- **Deteccion de plagio:** compara un documento sospechoso contra un corpus usando *matching statistics* sobre un suffix tree generalizado.

Stack: nucleo en C++ expuesto a Python con pybind11 → FastAPI.

---

## Modulo C++

Compilado con pybind11. El backend lo importa como `import suffix_tree_core as stc`.

```python
# Documento unico (Ctrl+F)
tree = stc.SuffixTree()
tree.build(text_normalized)           # C++ agrega \x00 internamente
tree.contains(pattern)                # bool
tree.count_occurrences(pattern)       # int
tree.find_occurrences(pattern)        # list[int]  — posiciones en texto normalizado
tree.get_tree_path(pattern)           # list[str]  — etiquetas de arista recorridas

# Corpus — arbol generalizado (max 254 docs, separadores internos \x01..\xFE)
corpus = stc.CorpusTree()
corpus.build([(text_norm, "doc1.pdf"), (text_norm2, "doc2.pdf")])
corpus.find_occurrences(pattern)      # list[Occurrence]  — .position, .source
corpus.matching_statistics(suspect_normalized, min_match_len=40)
# list[dict]  — [{pos, length, source}] solo donde length >= min_match_len

# Benchmark (mide internamente con chrono, corre `runs` veces)
stc.benchmark(text, pattern, runs=10)
# {suffix_tree_build_ns, suffix_tree_search_ns, naive_search_ns, occurrences}

# Busqueda bruta — oraculo para tests y baseline de benchmark
stc.naive_search(text, pattern)       # list[int]
```



---

## Carpetas de datos

```
backend/data/
├── search_doc/     # 1 archivo — el documento sobre el que se hace Ctrl+F y deteccion
├── corpus/         # N archivos — referencias contra las que se detecta plagio
└── benchmark/      # exactamente: benchmark_100k.txt, benchmark_500k.txt, benchmark_1M.txt
```

Los archivos entran por los endpoints (PDF o TXT). Python extrae el texto, lo normaliza y lo guarda en la carpeta correspondiente. Si una carpeta esta vacia cuando se necesita, el endpoint retorna error.

---

## Endpoints

El backend mantiene en memoria el `CorpusTree` (arbol de los N documentos de referencia), el `SuffixTree` del documento bajo analisis, su texto original y el `offset_map` para traducir posiciones normalizadas a originales.

---

### `POST /corpus/upload`
Recibe uno o varios archivos (PDF o TXT), extrae texto, normaliza y guarda en `data/corpus/`. No construye el arbol todavia.

Request: `multipart/form-data`, campo `files[]`.

Response: `{ "saved": ["ref1.txt", "ref2.pdf"], "total_chars": 142300 }`

---

### `POST /corpus/build`
Lee lo que haya en `data/corpus/` y construye el `CorpusTree` en memoria.

Response: `{ "sources": ["ref1.txt"], "total_chars": 142300, "build_time_ms": 87.4 }`

Errores: `corpus_dir_empty`, `corpus_too_large` (> 2 000 000 chars), `too_many_documents` (> 254).

---

### `POST /document/upload`
Recibe un archivo (PDF o TXT), extrae texto, normaliza y guarda en `data/search_doc/` (reemplaza cualquier archivo anterior).

Request: `multipart/form-data`, campo `file`.

Response: `{ "filename": "sospechoso.pdf", "char_count": 8400 }`

---

### `POST /document/load`
Lee el archivo de `data/search_doc/` y construye el `SuffixTree` en memoria.

Response: `{ "filename": "sospechoso.pdf", "text_original": "...", "char_count": 8400, "build_time_ms": 3.2 }`

El frontend guarda `text_original` para renderizar highlights.

Errores: `search_doc_missing`, `search_doc_multiple`.

---

### `POST /document/search`
Busca un patron en el documento cargado. Devuelve posiciones en texto **original**.

Request: `{ "pattern": "redes neuronales" }`

Response:
```json
{
  "occurrences": [142, 891],
  "count": 2,
  "tree_path": ["redes ", "neur", "onales"],
  "search_time_ms": 0.08
}
```

`tree_path`: aristas recorridas en el suffix tree (visualizacion educativa).

Errores: `no_document`, `empty_pattern`.

---

### `POST /detect`
Detecta plagio del documento cargado contra el corpus. Devuelve spans en texto **original**.

Request: `{ "min_match_length": 40 }`  (default: 40)

Response:
```json
{
  "spans": [
    { "start": 142, "end": 298, "source": "ref1.txt", "length": 156 }
  ],
  "global_pct": 18.4,
  "by_source": { "ref1.txt": 18.4 }
}
```

Agregacion de spans (en `corpus_manager.py`): fusionar solapados o con gap <= 5 chars del mismo source, calcular porcentajes sobre el largo del texto normalizado, traducir a coords originales con `offset_map`.

Errores: `no_corpus`, `no_document`.

---

### `POST /benchmark`
Corre el benchmark sobre los 3 archivos de `data/benchmark/`.

Request: `{ "pattern": "the" }`

Response:
```json
{
  "results": [
    { "size": 100000, "file": "benchmark_100k.txt",
      "suffix_tree_build_ms": 45.2, "suffix_tree_search_ms": 0.03,
      "naive_search_ms": 12.1, "occurrences": 847 },
    { "size": 500000, "...": "..." },
    { "size": 1000000, "...": "..." }
  ]
}
```

Error: `benchmark_file_missing` si algun archivo no existe.

---

## Frontend

SPA con tres pestañas construida con **Vite + vanilla JS** (sin framework). El frontend es un repositorio independiente.

| Pestaña | Funcionalidad |
|---|---|
| **Ctrl+F** | Subir PDF/TXT → cargar en suffix tree → buscar patrón → resalta ocurrencias + muestra ruta en el árbol |
| **Plagio** | Subir corpus → construir árbol generalizado → detectar plagio → spans coloreados por fuente con porcentajes |
| **Benchmark** | Ejecuta benchmark sobre 3 tamaños (100k, 500k, 1M) → tabla comparativa + gráfico de barras |

Inicialización:

```bash
cd frontend
pnpm install        # primera vez
pnpm dev            # http://localhost:5173
pnpm build          # produccion → dist/
```

El proxy de Vite redirige `/corpus`, `/document`, `/detect`, `/benchmark` y `/health` al backend.

---

## Decisiones tecnicas

**Algoritmo de Ukkonen:** el suffix tree se construye en O(n) tiempo y espacio usando el algoritmo de Ukkonen. La alternativa naive seria insertar cada sufijo uno a uno — O(n²). Ukkonen mantiene un "punto activo" y reglas de extension que permiten procesar cada caracter exactamente una vez.

**Terminal `\x00`:** al construir el arbol de un documento se agrega `\x00` al final del texto. Esto garantiza que todos los sufijos terminen en hojas distintas (sin `\x00` algunos sufijos podrian ser prefijos de otros y "perderse" dentro del arbol). El `\x00` nunca aparece en texto normalizado, asi que no hay colisiones.

**Arbol generalizado con separadores `\x01`–`\xFE`:** para el corpus de N documentos no se construyen N arboles separados — se concatenan todos con un byte separador unico por documento (`doc0 + \x01 + doc1 + \x02 + ...`) y se construye un solo arbol. Esto permite buscar un patron en todos los documentos a la vez en O(m). Limite de 254 documentos porque solo hay 254 valores disponibles (`\x01` a `\xFE`).

**Matching statistics (deteccion de plagio):** para cada posicion `i` del texto sospechoso se baja desde la raiz del arbol del corpus y se mide cuantos caracteres consecutivos de `sospechoso[i:]` aparecen en el corpus. El resultado es un arreglo de longitudes `ms[i]`. Posiciones donde `ms[i] >= min_match_length` son candidatas a plagio. Luego Python fusiona las posiciones adyacentes en spans.

**Normalizacion en Python, coordenadas en original:** el C++ trabaja siempre sobre texto normalizado (ASCII puro, sin acentos ni puntuacion). Python mantiene un `offset_map` que traduce cada posicion normalizada a su posicion en el texto original. Asi el frontend puede resaltar el texto tal como el usuario lo subio.

**`ñ` → `~`:** antes de quitar acentos con NFKD se reemplaza `ñ/N con tilde` por `~`. Sin esto, NFKD convertiria `ñ → n`, haciendo que "año" y "ano" sean identicos. 

---

## Comandos

```bash
# 1. Compilar el nucleo C++
uv run python build.py

# 2. Iniciar backend
cd backend && uv run uvicorn main:app
# http://localhost:8000

# 3. Iniciar frontend (otra terminal)
cd frontend && pnpm dev
# http://localhost:5173

# 4. Correr todos los tests (C++ + Python)
uv run python test.py
```
