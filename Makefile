
CXX := g++-16
CXXFLAGS := -Wall -std=c++20
INCLUDES := -Iinclude

TARGET := threadpool
SRC := examples/simple_thread_pool.cpp

TEST_TARGET := test_basic
TEST_SRC := tests/test_basic.cpp

BENCH_TARGET := run_bench
#BENCH_SRC := benchmarks/benchmark_threadpool.cpp
BENCH_SRC := benchmarks/bm_benchmark.cpp

PERFETTO_DIR := third_party/perfetto
PERFETTO_SRC := $(PERFETTO_DIR)/perfetto.cc
PERFETTO_OBJ := $(PERFETTO_DIR)/perfetto.o

.PHONY: all test benchmark clean

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
#	g++ -Wall -std=c++20 simple_thread_pool.cpp -o myThreadPool.exe

# build tests
test:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRC) -o $(TEST_TARGET) -fsanitize=thread -g -lpthread
	
	
benchmark: $(PERFETTO_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BENCH_SRC) -o $(BENCH_TARGET) -lbenchmark -lpthread -g -O3 -DNDEBUG
	
$(PERFETTO_OBJ): $(PERFETTO_SRC)
	$(CXX) $(CXXFLAGS) -I$(PERFETTO_DIR) -c $(PERFETTO_SRC) -o $(PERFETTO_OBJ)

# Plain debug build


# Release build - for performance testing




# clean artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(PERFETTO_OBJ)
