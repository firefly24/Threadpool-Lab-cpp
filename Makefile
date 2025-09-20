all:
	g++ -Wall -std=c++20 simple_thread_pool.cpp -o myThreadPool.exe
clean:
	rm -f myThreadPool.exe