#pragma once

#include <functional>


using Task = std::function<void()>;

inline void emptyTask()
{

}


inline void smallTask()
{
	volatile int x = 0;
	
	for (int i=0;i<1000; i++)
		x = x+1;
}


inline void mediumTask()
{
	volatile int x = 0;
	
	for (int i=0;i<100'000; i++)
		x = x+1;
}

inline void largeTask()
{
	volatile int x = 0;
	
	for (int i=0;i<1000'000; i++)
		x = x+1;
}
