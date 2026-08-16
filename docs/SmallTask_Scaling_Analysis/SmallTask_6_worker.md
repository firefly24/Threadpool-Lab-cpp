sudo "$(which perf)" stat -d ./run_bench   --benchmark_filter='BM_TaskScheduling/SmallTask/100000/6/100000'
2026-08-16T20:50:50+05:30
Running ./run_bench
Run on (6 X 1728 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB (x6)
  L1 Instruction 64 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 2048 KiB (x1)
Load Average: 0.13, 0.52, 0.63
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
BM_TaskScheduling/SmallTask/100000/6/100000/real_time  152181671 ns    114987808 ns            4 items_per_second=657.109k/s

 Performance counter stats for './run_bench --benchmark_filter=BM_TaskScheduling/SmallTask/100000/6/100000':

          4,235.64 msec task-clock                #    5.313 CPUs utilized          
          1,67,108      context-switches          #   39.453 K/sec                  
               348      cpu-migrations            #   82.160 /sec                   
               622      page-faults               #  146.849 /sec                   
    6,56,89,11,990      cycles                    #    1.551 GHz                    
    5,63,40,24,349      instructions              #    0.86  insn per cycle         
   <not supported>      branches                                                    
         50,95,631      branch-misses                                               
    2,02,38,09,448      L1-dcache-loads           #  477.804 M/sec                  
       1,71,20,270      L1-dcache-load-misses     #    0.85% of all L1-dcache accesses
       2,08,81,610      LLC-loads                 #    4.930 M/sec                  
         94,64,730      LLC-load-misses           #   45.33% of all LL-cache accesses

       0.797270847 seconds time elapsed

       2.930324000 seconds user
       1.336764000 seconds sys



