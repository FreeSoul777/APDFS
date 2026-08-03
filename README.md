**Read this in [Russian](README.ru.md)**

# APDFS: Adaptive Parallel Depth-First Search

APDFS is a high-performance algorithm for enumerating **all minimal (S,T)-cuts** in directed graphs. It implements parallel depth-first search with work-stealing load balancing.

## Build

### Dependencies

- CMake >= 3.20
- C++20 compiler (GCC, Clang, MSVC)
- POSIX Threads (Linux, macOS, WSL)

### Basic build

```bash
git clone https://github.com/FreeSoul777/APDFS.git
cd APDFS
git submodule update --init --recursive

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
./build/src/apdfs_cli <graph_file> <source> <sink> [threads] [output_dir] [--progress] [--max-cuts=N]
```

Parameters:
- `graph_file` — path to graph file
- `source` — super source index
- `sink` — super sink index
- `threads` — number of threads (default: hardware_concurrency)
- `output_dir` — output directory (default: `./output`)
- `--progress` — show progress
- `--max-cuts=N` — limit maximum number of cuts

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

## Contributing

Before creating a pull request, make sure you have completed the following steps:

1. **Code formatting**:
```bash
cmake --build build --target clang-format
```

2. **Static analysis** (if enabled):
```bash
cmake -S . -B build -DAPDFS_ENABLE_CLANG_TIDY=ON
cmake --build build -j$(nproc)
cmake --build build --target clang-tidy-check
```

3. **Tests**:
```bash
ctest --test-dir build --output-on-failure
```

4. **Describe the changes**: what was done, why, and what problems it solves