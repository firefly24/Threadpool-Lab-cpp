#!/bin/bash


PERF="$HOME/nvidia-sources/linux-jammy/tools/perf/perf"
FLAMEGRAPH_DIR="$HOME/tools/FlameGraph"
PROFILE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$PROFILE_DIR")"

BENCH="$PROJECT_ROOT/run_bench"

if [ $# -lt 1 ]; then
    echo "Usage:"
    echo "  $0 \"<benchmark_filter>\""
    exit 1
fi

FILTER="$1"

echo "=================================================="
echo "Recording profile..."
echo "=================================================="

sudo "$PERF" record \
    -F 999 \
    -g \
    -o "$PROFILE_DIR/perf.data" \
    "$BENCH" \
    --benchmark_filter="$FILTER" > $PROFILE_DIR/benchmark_results.txt

sudo chmod 777 "$PROFILE_DIR/perf.data"

echo
echo "=================================================="
echo "Converting perf.data..."
echo "=================================================="

sudo "$PERF" script -i "$PROFILE_DIR/perf.data" > out.perf

echo
echo "=================================================="
echo "Collapsing stacks..."
echo "=================================================="

"$FLAMEGRAPH_DIR"/stackcollapse-perf.pl \
    out.perf > out.folded

echo
echo "=================================================="
echo "Generating flame graph..."
echo "=================================================="

"$FLAMEGRAPH_DIR"/flamegraph.pl \
    out.folded > $PROFILE_DIR/flamegraph.svg

echo
echo "Done!"
echo
echo "Open:"
echo "    flamegraph.svg"
