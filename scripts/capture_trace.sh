#!/bin/bash

TRACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$TRACE_DIR")"

TRACE="$TRACE_DIR/threadpool-sched.perfetto-trace"
PERFETTO="$HOME/perfetto/out/linux/perfetto"
CONFIG="$HOME/perfetto/out/linux/sched.cfg"

sudo "$PERFETTO" --txt -c "$CONFIG" -o "$TRACE" &
TRACE_PID=$!

sleep 1

$PROJECT_ROOT/run_bench \
  --benchmark_filter=BM_TaskScheduling/EmptyTask/100000/4/100000

wait "$TRACE_PID"

sudo chown "$USER:$USER" "$TRACE"

echo "Trace written to: $TRACE"
