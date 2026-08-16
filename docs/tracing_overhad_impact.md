EXP-001 — Initial tracing overhead result

For the 4-worker configuration:

Workload	Tracing OFF	Tracing ON	Approx. overhead
EmptyTask	158.1 ms	232.2 ms	+46.8%
SmallTask	155.6 ms	303.4 ms	+95.0%
MediumTask	7340.4 ms	7504.3 ms	+2.2%



Data used: 
Tracing overhead seems to not matter for medium sized tasks, whereas for smaller tasks, tracing overhead contributes signigicantly
For with tracing: 
BM_TaskScheduling/EmptyTask/100000/4/100000/real_time  232152475 ns     26278411 ns            3 items_per_second=430.751k/s
BM_TaskScheduling/SmallTask/100000/4/100000/real_time  303380479 ns     13518352 ns            2 items_per_second=329.619k/s
BM_TaskScheduling/MediumTask/100000/4/100000/real_time 7504321108 ns      9289792 ns            1 items_per_second=13.3257k/s


With trace disabled: 
BM_TaskScheduling/EmptyTask/100000/4/100000/real_time  158120513 ns    150786221 ns            5 items_per_second=632.429k/s
BM_TaskScheduling/SmallTask/100000/4/100000/real_time  155566954 ns     94445606 ns            5 items_per_second=642.81k/s
BM_TaskScheduling/MediumTask/100000/4/100000/real_time 7340420854 ns     10179520 ns            1 items_per_second=13.6232k/s

