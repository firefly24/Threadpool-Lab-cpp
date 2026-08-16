 sudo "$(which perf)" stat -d ./run_bench   --benchmark_filter='BM_TaskScheduling/SmallTask/100000/1/100000'
2026-08-16T20:48:09+05:30
Running ./run_bench
Run on (6 X 1728 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB (x6)
  L1 Instruction 64 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 2048 KiB (x1)
Load Average: 0.59, 0.78, 0.73
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
BM_TaskScheduling/SmallTask/100000/1/100000/real_time  306833784 ns      9251936 ns            2 items_per_second=325.909k/s

 Performance counter stats for './run_bench --benchmark_filter=BM_TaskScheduling/SmallTask/100000/1/100000':

            945.26 msec task-clock                #    1.024 CPUs utilized          
               403      context-switches          #  426.336 /sec                   
                 1      cpu-migrations            #    1.058 /sec                   
               952      page-faults               #    1.007 K/sec                  
    1,59,49,29,387      cycles                    #    1.687 GHz                    
    1,67,05,43,949      instructions              #    1.05  insn per cycle         
   <not supported>      branches                                                    
          4,28,729      branch-misses                                               
      65,92,97,090      L1-dcache-loads           #  697.474 M/sec                  
          5,11,346      L1-dcache-load-misses     #    0.08% of all L1-dcache accesses
          7,16,222      LLC-loads                 #  757.695 K/sec                  
          3,84,039      LLC-load-misses           #   53.62% of all LL-cache accesses

       0.923264122 seconds time elapsed

       0.927240000 seconds user
       0.015917000 seconds sys


