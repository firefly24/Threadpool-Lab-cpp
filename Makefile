
CXX := g++-16
CXXFLAGS := -Wall -std=c++20
INCLUDES := -Iinclude

TARGET := threadpool
SRC := examples/simple_thread_pool.cpp

TEST_TARGET := test_basic
TEST_SRC := tests/test_basic.cpp

BENCH_TARGET := run_bench
BENCH_TARGET_TRACED := run_bench_traced
BENCH_TARGET_PERF := run_bench-perf
#BENCH_SRC := benchmarks/benchmark_threadpool.cpp
BENCH_SRC := benchmarks/bm_benchmark.cpp

PERFETTO_DIR := third_party/perfetto
PERFETTO_SRC := $(PERFETTO_DIR)/perfetto.cc
PERFETTO_OBJ := perfetto.o

TRACING_SRC := src/instrumentation/tracing.cpp
TRACING_OBJ := tracing.o
NOTRACING_OBJ := notracing.o

.PHONY: all test benchmark benchmark-traced clean

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
#	g++ -Wall -std=c++20 simple_thread_pool.cpp -o myThreadPool.exe

# build tests
test: $(NOTRACING_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRC) $(NOTRACING_OBJ) -o $(TEST_TARGET) -fsanitize=thread -g -O3 -lpthread
	
# Release build - for performance testing
benchmark: $(NOTRACING_OBJ) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BENCH_SRC) $(NOTRACING_OBJ) -o $(BENCH_TARGET) -lbenchmark -lpthread -g -O1 -fno-omit-frame-pointer -fno-inline #-fsanitize=thread -O3 -DNDEBUG 
	
benchmark-perf: $(NOTRACING_OBJ) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BENCH_SRC) $(NOTRACING_OBJ) -o $(BENCH_TARGET_PERF) -lbenchmark -lpthread -g -O3 -DNDEBUG 
	
# For testing with custom traces enabled
benchmark-traced: $(TRACING_OBJ) $(PERFETTO_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES)  -I$(PERFETTO_DIR) -DTHREADPOOL_ENABLE_TRACING $(BENCH_SRC) $(TRACING_OBJ) $(PERFETTO_OBJ) -o $(BENCH_TARGET_TRACED) -lbenchmark -lpthread -g -O3

# For tracing enabled
$(TRACING_OBJ): $(TRACING_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(PERFETTO_DIR) -c $(TRACING_SRC) -o $(TRACING_OBJ) -DTHREADPOOL_ENABLE_TRACING
	
# For tracing disabled
$(NOTRACING_OBJ): $(TRACING_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(PERFETTO_DIR) -c $(TRACING_SRC) -o $(NOTRACING_OBJ)

	
$(PERFETTO_OBJ): $(PERFETTO_SRC)
	$(CXX) $(CXXFLAGS) -I$(PERFETTO_DIR) -c $(PERFETTO_SRC) -o $(PERFETTO_OBJ)


# Plain debug build

# clean artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(PERFETTO_OBJ)  $(TRACING_OBJ) $(NOTRACING_OBJ) $(BENCH_TARGET_TRACED) $(BENCH_TARGET_PERF)
