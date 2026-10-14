# DevFinder Architecture

Initial scaffold for the DevFinder search and ranking engine.

## Phase 1: Core Data Models & Config
- Added `RepoMetadata` for storing parsed repository information with JSON serialization support.
- Added `Config` class to manage application settings loaded from `.env` files with strict error handling on missing files.

## Phase 2: GitHub Ingestion Module
- Implemented `GithubClient` utilizing `libcurl` for HTTP requests, propagating rate limits as `RateLimitError`.
- Added `RepoFetcher` to retrieve paginated results from GitHub REST API, parse them into `RepoMetadata`, and verify DevOps readiness properties.
- Configured CLI and shell scripts to dispatch fetch commands and persist the output as raw JSON on disk.

## Phase 3: Text Preprocessing / Tokenizer
- Added `Tokenizer` module responsible for stripping punctuation, splitting by whitespace, and splitting camelCase, snake_case, and kebab-case.
- Included an embedded stopword filter to strip common meaningless words, emitting normalized and cleanly tokenized text elements for the inverted index.

## Phase 4: Inverted Index & Storage
- Implemented `InvertedIndex` handling term frequencies, document counting, and tracking exact positional data for each term.
- Built a highly optimized `IndexStorage` layer allowing instant binary dumps to disk and seamless reloading using a custom, length-prefixed binary serialization format with no external libraries.

## Phase 5: BM25 Ranking Engine
- Built `Bm25Ranker` to compute industry-standard BM25 relevance scores.
- Implemented safe IDF calculations and term-frequency saturation, returning properly descending sorted results without modifying the underlying index state.

## Phase 6: Composite Health Score
- Built `HealthScorer` that merges abstract BM25 scores with tangible repository metrics (`stars`, `forks`, `last_commit_date`, and DevOps flags).
- Utilized log-scaled popularity curves and exponential decay mechanics for active maintenance evaluations, providing an explanatory multidimensional breakdown structure instead of just returning a flat float score.

## Phase 7: Query Engine & Filters
- Created `QueryParser` to split free-text inputs from semantic structured filters (`language:`, `has:docker`, `stars:>`, `updated:<`).
- Orchestrated the full retrieval pipeline within `QueryEngine`, seamlessly integrating tokenization, BM25 scoring, structured boolean post-filtering, and final composited health scoring into a high-performance ranked response.

## Phase 8: CLI Interface
- Rewrote `main_cli.cpp` to expose `fetch`, `index`, and `search` subcommands.
- Formatted search output to visually expose the internal score breakdown metrics out-of-the-box, providing essential explainability into why a repository is ranked highly. 

## Phase 9: HTTP REST API Server
- Wrapped the internal engine behind `cpp-httplib`, exposing `/health`, `/api/search`, and `/api/autocomplete` REST endpoints natively in C++.
- Encapsulated parameter checking, mapped internal queries to formatted JSON, and strictly prevented internal exceptions from leaking into raw HTTP 500 payloads.

## Phase 10: Web Frontend
- Implemented a minimalist, text-first UI explicitly constrained to two stark themes (Light/Dark).
- Designed the UI using zero emojis, zero neon colors, and zero extraneous shadows/animations.
- Built a functional search and filter layout directly communicating with the Phase 9 REST backend.
- Implemented an expandable health-score breakdown natively utilizing CSS variables to construct visually consistent sub-score bars mapped to a muted, professional accent palette (slate blue, soft amber, teal, coral).

### Manual Acceptance Checklist (Phase 10)
- [x] Theme choice (Light/Dark) persists after page reload via LocalStorage.
- [x] UI remains entirely legible in both themes (text contrasts cleanly with backgrounds).
- [x] Zero emoji are present anywhere in the DOM.
- [x] Zero neon or highly saturated colors exist outside of the isolated health-score breakdown bars.
- [x] Browser search output rigorously matches Phase 8's CLI output rankings for the same text query.

## Phase 11: Autocomplete (Trie)
- Engineered a lightweight `Trie` datastructure utilizing `std::unordered_map` and smart pointers for deterministic O(1) memory cleanup and sub-millisecond prefix matching.
- Wired Trie generation directly into `devfinder index`, iterating tokens, languages, and topics from the raw JSON payload to accumulate dynamic term weights (corpus frequencies).
- Integrated `trie_terms.txt` directly into `devfinder serve`, successfully binding `/api/autocomplete` to rank exact term completions via their term-frequency weights instantaneously.
