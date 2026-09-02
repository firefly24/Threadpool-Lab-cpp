#!/bin/bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
	echo "Usage:"
    echo "  $0 \"<Workload>\" \"<Num Workers>\" \"<Batch size>\""
    exit 1
fi

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPTS_DIR")"
PERF_BIN="$(which perf)"

BENCH="run_bench-perf"

WORKLOAD=$1
WORKERS=$2
BATCH_SIZE=$3

if [ ! -x "$PROJECT_ROOT/$BENCH" ]; then
	echo "=================================================="
	echo "Builing benchmark binary..."
	echo "=================================================="
	make benchmark-perf -C "$PROJECT_ROOT"
fi

FILTER="BM_TaskScheduling/$WORKLOAD/100000/$BATCH_SIZE/100000/$WORKERS"


TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

RESULTS_DIR="$SCRIPTS_DIR/results/PERF_STAT_${TIMESTAMP}"

mkdir -p "$RESULTS_DIR"

echo "Benchmark results will be captured in $RESULTS_DIR "

sudo "$PERF_BIN" stat \
    -d -d \
    "$PROJECT_ROOT/$BENCH" \
    --benchmark_filter="$FILTER" \
    --benchmark_repetitions=1 \
    > "$RESULTS_DIR/benchmark.log" 2>&1
