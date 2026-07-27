
CXX := g++
CXXFLAGS := -Wall -std=c++20
INCLUDES := -Iinclude

TARGET := threadpool

SRC := examples/simple_thread_pool.cpp

all:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
#	g++ -Wall -std=c++20 simple_thread_pool.cpp -o myThreadPool.exe


# Plain debug build


# Release build - for performance testing


# clean artifacts
clean:
	rm -f $(TARGET)
