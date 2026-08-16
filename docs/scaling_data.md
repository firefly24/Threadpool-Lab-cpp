-----------------------------------------------------------------------------------------------------------------
Benchmark                                                       Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------------------------------------
BM_TaskScheduling/EmptyTask/100000/1/100000/real_time    57998159 ns     55695936 ns           12 items_per_second=1.72419M/s
BM_TaskScheduling/EmptyTask/100000/2/100000/real_time   189447325 ns    178802931 ns            5 items_per_second=527.851k/s
BM_TaskScheduling/EmptyTask/100000/4/100000/real_time   198541292 ns    189764608 ns            4 items_per_second=503.674k/s
BM_TaskScheduling/EmptyTask/100000/6/100000/real_time   540796948 ns    296083776 ns            1 items_per_second=184.912k/s
BM_TaskScheduling/EmptyTask/100000/8/100000/real_time   242926067 ns    206990475 ns            3 items_per_second=411.648k/s
BM_TaskScheduling/SmallTask/100000/1/100000/real_time   307537734 ns     11285088 ns            2 items_per_second=325.163k/s
BM_TaskScheduling/SmallTask/100000/2/100000/real_time   231508013 ns     17492032 ns            3 items_per_second=431.95k/s
BM_TaskScheduling/SmallTask/100000/4/100000/real_time   166155033 ns     96368776 ns            4 items_per_second=601.848k/s
BM_TaskScheduling/SmallTask/100000/6/100000/real_time   264221456 ns    158115032 ns            4 items_per_second=378.47k/s
BM_TaskScheduling/SmallTask/100000/8/100000/real_time   197750273 ns    147823784 ns            4 items_per_second=505.688k/s
BM_TaskScheduling/MediumTask/100000/1/100000/real_time 29018828140 ns      6695232 ns            1 items_per_second=3.44604k/s
BM_TaskScheduling/MediumTask/100000/2/100000/real_time 14576324599 ns      6729408 ns            1 items_per_second=6.86044k/s
BM_TaskScheduling/MediumTask/100000/4/100000/real_time 7285229493 ns      8101664 ns            1 items_per_second=13.7264k/s
BM_TaskScheduling/MediumTask/100000/6/100000/real_time 5177764225 ns      9157440 ns            1 items_per_second=19.3134k/s
BM_TaskScheduling/MediumTask/100000/8/100000/real_time 5206268733 ns      7855040 ns            1 items_per_second=19.2076k/s


Using real_time as the performance measurement, the speedup relative to one worker is:

Workload	2 workers	4 workers	6 workers	8 workers
Empty		0.31×		0.29×		0.11×		0.24×
Small		1.33×		1.85×		1.16×		1.56×
Medium		1.99×		3.98×		5.60×		5.57×

And scaling efficiency:

Workload	2 workers	4 workers	6 workers	8 workers
Empty		15.3%		7.3%		1.8%		3.0%
Small		66.4%		46.3%		19.4%		19.4%
Medium		99.5%		99.6%		93.4%		69.7%


Scaling results w.r.t throughput : 

Workload:
EmptyTask
		Workers	Throughput	Speedup Scaling efficiency
			1	1.724 M/s	1.00×	100%
			2	527.9 k/s	0.31×	15.3%
			4	503.7 k/s	0.29×	7.3%
			6	184.9 k/s	0.11×	1.8%
			8	411.6 k/s	0.24×	3.0%
			
Small	
		Workers	Throughput	Speedup Scaling efficiency
			1	325.2 k/s	1.00×	100%
			2	432.0 k/s	1.33×	66.4%
			4	601.8 k/s	1.85×	46.3%
			6	378.5 k/s	1.16×	19.4%
			8	505.7 k/s	1.56×	19.4%
	
Medium	
		Workers	Throughput	Speedup Scaling efficiency
			1	3.446 k/s	1.00×	100%
			2	6.860 k/s	1.99×	99.5%
			4	13.726 k/s	3.98×	99.6%
			6	19.313 k/s	5.60×	93.4%
			8	19.208 k/s	5.57×	69.7%











