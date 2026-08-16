sudo "$(which perf)" stat -d ./run_bench   --benchmark_filter='BM_TaskScheduling/SmallTask/100000/4/100000'
2026-08-16T20:49:37+05:30
Running ./run_bench
Run on (6 X 1728 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB (x6)
  L1 Instruction 64 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 2048 KiB (x1)
Load Average: 0.31, 0.65, 0.68
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
BM_TaskScheduling/SmallTask/100000/4/100000/real_time  149952912 ns     80343661 ns            5 items_per_second=666.876k/s

 Performance counter stats for './run_bench --benchmark_filter=BM_TaskScheduling/SmallTask/100000/4/100000':

          3,752.02 msec task-clock                #    4.165 CPUs utilized          
            35,759      context-switches          #    9.531 K/sec                  
                70      cpu-migrations            #   18.657 /sec                   
             1,257      page-faults               #  335.019 /sec                   
    5,41,92,31,309      cycles                    #    1.444 GHz                    
    4,16,33,85,425      instructions              #    0.77  insn per cycle         
   <not supported>      branches                                                    
         34,88,615      branch-misses                                               
    1,64,54,83,497      L1-dcache-loads           #  438.559 M/sec                  
       1,50,46,921      L1-dcache-load-misses     #    0.91% of all L1-dcache accesses
       1,77,21,943      LLC-loads                 #    4.723 M/sec                  
         80,09,711      LLC-load-misses           #   45.20% of all LL-cache accesses

       0.900776273 seconds time elapsed

       3.142359000 seconds user
       0.632848000 seconds sys



