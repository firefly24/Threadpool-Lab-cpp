#!/bin/bash

set -euo pipefail

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPTS_DIR")"

BENCH="run_bench-perf"
REPETITIONS=5

if [ ! -x "$PROJECT_ROOT/$BENCH" ]; then
	echo "=================================================="
	echo "Builing benchmark binary..."
	echo "=================================================="
	make benchmark-perf -C "$PROJECT_ROOT"
fi

echo "=================================================="
echo "Running Benchmark..."
echo "=================================================="

FILTER="BM_TaskScheduling/"

if [ "$#" -lt 1 ]; then
	echo "Running full benchmark" >&2
	FILTER="BM_TaskScheduling"
fi

FILTER="BM_TaskScheduling/"

if [ "$#" -eq 1 ]; then
	WORKLOAD=$1
	echo "Running full benchmark for workload: $WORKLOAD" >&2
	FILTER="BM_TaskScheduling/$WORKLOAD"
fi 

if [ "$#" -eq 2 ]; then
	WORKLOAD=$1
	WORKERS=$2
	echo "Running benchmark for workload: $WORKLOAD, with Workers: $WORKERS" >&2
	FILTER="BM_TaskScheduling/$WORKLOAD/100000/$WORKERS/100000"
fi

if [ "$#" -eq 3 ]; then
	WORKLOAD=$1
	WORKERS=$2
	BATCH_SIZE=$3
	echo "Running benchmark for workload: $WORKLOAD, with Workers: $WORKERS, batch:$BATCH_SIZE" >&2
	FILTER="BM_TaskScheduling/$WORKLOAD/100000/$WORKERS/100000/$BATCH_SIZE"
fi

TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

RESULTS_DIR="$SCRIPTS_DIR/results/BENCH_${TIMESTAMP}"

mkdir -p "$RESULTS_DIR"

echo "Benchmark results will be captured in $RESULTS_DIR "

"$PROJECT_ROOT/$BENCH" \
--benchmark_filter="$FILTER" > "$RESULTS_DIR/benchmark.log" \
--benchmark_out="$RESULTS_DIR/results.json" \
--benchmark_out_format=json
	
echo "Done!"

METADATA_FILE="$RESULTS_DIR/metadata.txt"

{
    echo "timestamp=$TIMESTAMP"
    echo "git_commit=$(git -C "$PROJECT_ROOT" rev-parse HEAD)"
    echo "git_status:"
    git -C "$PROJECT_ROOT" status --short
    echo

    echo "system:"
    uname -a
    echo

    echo "compiler:"
    g++-16 --version | head -n 1
    echo

    echo "benchmark_binary=$PROJECT_ROOT/$BENCH"
    echo "benchmark_filter=$FILTER"
    #echo "benchmark_repetitions=$REPETITIONS"
} > "$METADATA_FILE"

#python3 "$SCRIPTS_DIR/visualize_batching.py" "$RESULTS_DIR/benchmark.log"


