
Sample workload used: 

inline void genericTask()
{
	volatile int x = 0;
	
	for (int i=0;i<K; i++)
		x = x+1;
}


| Workload  | K		| Observed behavior                                               |
| ----------|-------|---------------------------------------------------------------- |
| Empty     | 0		| Negative scaling - scheduling/synchronization dominates         |
| Small     |1000 	| Improves to ~4 workers, then degrades/plateaus                  |
| Medium    |100000 | Almost linear scaling through 6 workers                         |
| Large     |1000000| Almost linear scaling through 6 workers; confirms Medium result |
| 8 workers | 	-	| No benefit for compute-heavy work on the reported 6-CPU system  |
