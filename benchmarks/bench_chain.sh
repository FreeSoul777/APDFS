#!/bin/bash
BUILD_DIR="${1:-build}"
GRAPHGEN="${BUILD_DIR}/tools/graphgen"
SOLVER="${BUILD_DIR}/src/app/apdfs_cli"

if [ ! -x "$GRAPHGEN" ] || [ ! -x "$SOLVER" ]; then
    echo "Build project: cd $BUILD_DIR && cmake --build . -j\$(nproc)"
    exit 1
fi

echo "=== Chain of Diamonds Benchmark ==="
echo ""
printf "%-4s %-10s %-10s %-12s %-10s %-10s\n" \
    "K" "Vertices" "Edges" "Cuts" "Time(s)" "Memory(MB)"
printf "%-4s %-10s %-10s %-12s %-10s %-10s\n" \
    "---" "--------" "-----" "-----------" "-------" "----------"

for K in 1 2 3 4 5 6 7 8 9 10; do
    GRAPH_FILE="outdir/chain_${K}.txt"
    OUTPUT_DIR="outdir/chain_${K}_out"
    rm -rf "$OUTPUT_DIR"

    $GRAPHGEN chain "$K" "$GRAPH_FILE" > /dev/null 2>&1

    OUTPUT=$($SOLVER "$GRAPH_FILE" 0 $((3*K)) 1 "$OUTPUT_DIR" 2>&1)
    
    CUTS=$(echo "$OUTPUT" | grep "Total cuts:" | awk '{print $3}')
    TIME=$(echo "$OUTPUT" | grep "Duration:" | awk '{print $2}')
    MEM=$(echo "$OUTPUT" | grep "Memory:" | awk '{print $2}')
    VERTICES=$(echo "$OUTPUT" | grep "Graph:" | awk '{print $2}')
    EDGES=$(echo "$OUTPUT" | grep "Graph:" | awk '{print $4}')
    
    printf "%-4d %-10s %-10s %-12s %-10s %-10s\n" \
        "$K" "$VERTICES" "$EDGES" "$CUTS" "$TIME" "$MEM"

    rm -f "$GRAPH_FILE"
    rm -rf "$OUTPUT_DIR"
done

echo ""
echo "Formula: C(K) = 4*K"
echo "K=1: 4, K=2: 8, K=3: 12, K=4: 16, K=5: 20, ..."
