**Read this in [Russian](README.ru.md)**

# APDFS: Adaptive Parallel Depth-First Search

APDFS is a high-performance algorithm for enumerating **all minimal (S,T)-cuts** in directed graphs. It implements parallel depth-first search with work-stealing load balancing.

## Build

### Dependencies

- Linux (x86-64). Only Linux is supported: the code uses `<immintrin.h>`/`_mm_pause` and
  `<sys/resource.h>`/`getrusage`
- CMake >= 3.20
- C++20 compiler: GCC or Clang (MSVC is not supported)
- POSIX threads (glibc)

### Basic build

```bash
git clone https://github.com/FreeSoul777/APDFS.git
cd APDFS

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Build options

| Option | Default | Description |
|-------|---------|-------------|
| `APDFS_BUILD_TESTS` | ON | Build unit tests |
| `APDFS_BUILD_BENCHMARKS` | OFF | Build benchmarks |
| `APDFS_BUILD_TOOLS` | ON | Build tools (graphgen, cutreader) |
| `APDFS_ENABLE_CLANG_FORMAT` | ON | Enable clang-format target |
| `APDFS_ENABLE_CLANG_TIDY` | OFF | Enable clang-tidy target |

Full build example:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DAPDFS_BUILD_TESTS=ON \
    -DAPDFS_BUILD_BENCHMARKS=ON \
    -DAPDFS_BUILD_TOOLS=ON

cmake --build build -j$(nproc)
```

## Usage

### Running

```bash
./build/src/apdfs_cli <graph_file> <source> <sink> [threads] [output_dir] [--progress] [--max-cuts=N] [--max-cut-size=N] [--max-examined=N]
```

Parameter reference (`apdfs --help` prints the same text):

| Argument | Meaning | When you need it |
|---|---|---|
| `graph_file` | text graph: `# comments`, `<vertices> <edges>`, then one `<u> <v>` per edge | always |
| `source`, `sink` | vertex ids of `S*` and `T*` | always |
| `threads` | worker threads, default hardware concurrency | pin it for reproducible timings |
| `output_dir` | output directory, default `./output` | |
| `--threads=N` | same as the positional `threads` | explicit flags override positionals |
| `--output=DIR` | same as the positional `output_dir` | explicit flags override positionals |
| `--progress` | live line: cuts, duplicates, heap, memory, ETA | long runs |
| `--max-cuts=N` | stop after `N` **emitted** cuts | early stop on huge outputs |
| `--max-cut-size=N` | emit only cuts with at most `N` edges | small cut sets are the interesting ones in reliability |
| `--max-examined=N` | stop after `N` **examined** cuts (emitted + filtered) | the only bounded stop when `--max-cut-size` filters almost everything |
| `-h`, `--help` | usage text and exit codes | |
| `--version` | program version | |

Two flags look symmetrical but are not. `--max-cut-size` is an **output filter**: the search still
visits larger cuts, because a large cut can have small descendants, so completeness is guaranteed only
for cuts within the bound. `--max-cuts` counts **emitted** cuts, which is why a size-filtered run needs
`--max-examined` to stop at all. Unknown options, missing values and non-numeric values are rejected
with exit code 2 rather than silently ignored.

Example:

```bash
./build/src/apdfs_cli graph.txt 0 99 4 output --progress
```

### Input file format

```
# Comment
<vertices> <edges>
<u> <v>
<u> <v>
...
```

Example:

```
# Simple graph
4 5
0 1
0 2
1 3
2 3
1 2
```

## Testing

```bash
# Build with tests
cmake -S . -B build -DAPDFS_BUILD_TESTS=ON
cmake --build build -j$(nproc)

# Run all tests
cd build
ctest --output-on-failure

# Run specific test
./tests/apdfs_tests --gtest_filter=TestBfsEngine.*
```

## Benchmarks

Benchmarks measure algorithm performance on different graphs with varying thread counts. Uses Google Benchmark.

### Running

```bash
# Build with benchmarks
cmake -S . -B build -DAPDFS_BUILD_BENCHMARKS=ON
cmake --build build -j$(nproc)

# All benchmarks
./build/benchmarks/bench_apdfs

# Specific benchmark
./build/benchmarks/bench_apdfs --benchmark_filter=Chain

# Single-threaded only
./build/benchmarks/bench_apdfs --benchmark_filter=Chain1T

# Four-threaded only
./build/benchmarks/bench_apdfs --benchmark_filter=Chain4T

# Save results
./build/benchmarks/bench_apdfs --benchmark_out=results.json --benchmark_out_format=json
```

### What is measured

- **Chain1T/Chain4T** — chains of diamonds (K=10, 12, 15, 20) with 1 and 4 threads
- **Random1T/Random4T** — random graph (20 vertices, 60 edges) with 1 and 4 threads

## Measured performance

Reference numbers on one machine, to give a sense of throughput and memory. These are *not*
reproducible guarantees (they depend on the CPU, the graph, and the operating system); treat them as
an order-of-magnitude guide and re-measure before citing.

**Machine:** AMD Ryzen AI 9 365 (10 cores / 20 threads), 26 GiB RAM, GCC 15.2, `Release` (`-O3`), Ninja.

**Workload:** `graphgen random 300 1200 0.2 0.2` (300 vertices, 1200 edges), `S*=0`, `T*=299`,
`--max-cuts=1000000` (≈210–227 MB of `.bin` output).

| Threads | Throughput | Speedup | Peak RSS |
|--------:|-----------:|--------:|---------:|
| 1  | 106 142 cuts/s | 1.00× |  54 MB |
| 4  | 333 145 cuts/s | 3.14× |  94 MB |
| 8  | 496 732 cuts/s | 4.68× | 148 MB |
| 16 | 632 386 cuts/s | 5.96× | 254 MB |

Scaling flattens past 8 threads (the global `SeenTable` and memory bandwidth become the bottleneck).

**Memory is linear in the number of emitted cuts** (the `SeenTable` is `Θ(|𝔻|)`), not in the graph:

| Emitted cuts (4 threads) | Peak RSS |
|-------------------------:|---------:|
| 100 000  |  60 MB |
| 250 000  |  66 MB |
| 500 000  |  78 MB |
| 1 000 000 |  93 MB |

That is ≈39 bytes per cut (a 16-byte fingerprint slot at 0.5 load factor plus overhead). On tiny
graphs (≤ 100 cuts) parallel runs are *slower* than single-threaded ones because thread startup
(≈1 ms) dominates the useful work — see the `Chain*T` benchmarks.

## Tools

### Graph generator

```bash
./build/tools/graphgen <mode> [parameters]
```

Modes:

**chain** — chain of K "diamonds" (sequential connection of rhombi):
```bash
./build/tools/graphgen chain <K> [output_file]
```
- `K` — number of diamonds in the chain
- Each diamond adds 3 vertices and 4 edges
- Default: `chain_K.txt`

**random** — random graph with main path 0→1→...→(n-1) and additional edges:
```bash
./build/tools/graphgen random <vertices> <edges> <side_rate> <cycle_rate> [output_file]
```
- `vertices` — number of vertices
- `edges` — number of edges
- `side_rate` — fraction of side branches (forward edges, non-adjacent)
- `cycle_rate` — fraction of backward edges (create cycles)
- Default: `random_V_E.txt`

Example:
```bash
# Chain of 5 diamonds
./build/tools/graphgen chain 5

# Random graph: 20 vertices, 60 edges, 20% branches, 20% cycles
./build/tools/graphgen random 20 60 0.2 0.2
```

### Cut reader

```bash
./build/tools/cutreader <output_dir> [--page N] [--human]
```

Parameters:
- `output_dir` — directory with `.bin` cut files
- `--page N` — page through output, N cuts at a time
- `--human` — show edges as `source->target` (requires `edges.map` in output_dir)

Example:
```bash
# All cuts
./build/tools/cutreader output

# Page through 50 cuts at a time
./build/tools/cutreader output --page 50

# Human-readable format
./build/tools/cutreader output --human
```

## Output format

The algorithm writes cuts to binary `.bin` files in the specified directory. Each file contains a sequence of cuts.

## Algorithm

APDFS implements parallel DFS over the implicit graph of minimal cuts. Detailed algorithm description, correctness and completeness proofs: [docs/APDFS_SPECIFICATION.md](docs/APDFS_SPECIFICATION.md)

## License

APDFS is licensed under the Apache License, Version 2.0.

Copyright 2026 FreeSoul777

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Author

**FreeSoul777** (https://github.com/FreeSoul777)

## Citation

If you use APDFS in your research, please cite:

```bibtex
@software{apdfs2026,
  author = {FreeSoul777},
  title = {APDFS: Adaptive Parallel Depth-First Search for Enumerating All Minimal (S,T)-Cuts},
  year = {2026},
  url = {https://github.com/FreeSoul777/APDFS}
}
```

## Contributing

Before creating a pull request, make sure you have completed the following steps:

1. **Code formatting**:
```bash
cmake --build build --target clang-format-check
```

2. **Static analysis**:
```bash
cmake --build build --target clang-tidy-check
```

3. **Tests**:
```bash
ctest --test-dir build --output-on-failure
```

4. **Describe the changes**: what was done, why, and what problems it solves