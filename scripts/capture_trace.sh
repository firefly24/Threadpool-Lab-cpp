#!/bin/bash

set -euo pipefail

TRACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$TRACE_DIR")"

TRACE="$TRACE_DIR/threadpool-sched.perfetto-trace"
PERFETTO="$HOME/perfetto/out/linux/perfetto"
CONFIG="$HOME/perfetto/out/linux/sched.cfg"

BENCH="run_bench_traced"
WORKERS=$1
BATCH_SIZE=$2

sudo "$PERFETTO" --txt -c "$CONFIG" -o "$TRACE" &
TRACE_PID=$! && \
$PROJECT_ROOT/$BENCH \
  --benchmark_filter=BM_TaskScheduling/SmallTask/100000/$WORKERS/100000/$BATCH_SIZE

wait "$TRACE_PID"

sudo chown "$USER:$USER" "$TRACE"

echo "Trace written to: $TRACE"
