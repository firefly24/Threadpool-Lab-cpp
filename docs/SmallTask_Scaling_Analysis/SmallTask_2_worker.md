sudo "$(which perf)" stat -d ./run_bench   --benchmark_filter='BM_TaskScheduling/SmallTask/100000/2/100000'
2026-08-16T20:48:39+05:30
Running ./run_bench
Run on (6 X 1728 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB (x6)
  L1 Instruction 64 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 2048 KiB (x1)
Load Average: 0.58, 0.76, 0.72
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
BM_TaskScheduling/SmallTask/100000/2/100000/real_time  186470540 ns     13731744 ns            3 items_per_second=536.278k/s

 Performance counter stats for './run_bench --benchmark_filter=BM_TaskScheduling/SmallTask/100000/2/100000':

          1,608.82 msec task-clock                #    2.033 CPUs utilized          
             1,088      context-switches          #  676.271 /sec                   
                 3      cpu-migrations            #    1.865 /sec                   
               943      page-faults               #  586.143 /sec                   
    2,58,59,76,648      cycles                    #    1.607 GHz                    
    2,25,96,35,757      instructions              #    0.87  insn per cycle         
   <not supported>      branches                                                    
          7,00,774      branch-misses                                               
      93,09,79,665      L1-dcache-loads           #  578.671 M/sec                  
         50,55,214      L1-dcache-load-misses     #    0.54% of all L1-dcache accesses
         57,02,001      LLC-loads                 #    3.544 M/sec                  
         23,61,967      LLC-load-misses           #   41.42% of all LL-cache accesses

       0.791366049 seconds time elapsed

       1.556778000 seconds user
       0.035510000 seconds sys



