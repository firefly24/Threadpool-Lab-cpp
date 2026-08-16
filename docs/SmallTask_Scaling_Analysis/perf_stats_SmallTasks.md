| Metric                 |    1 worker |  2 workers |    4 workers |   6 workers |
| ---------------------- | ----------: | ---------: | -----------: | ----------: |
| **Throughput**         |    325.9k/s |   536.3k/s | **666.9k/s** |    657.1k/s |
| **Speedup**            |       1.00× |      1.65× |    **2.05×** |       2.02× |
| **Scaling efficiency** |        100% |      82.3% |        51.2% |       33.6% |
| Benchmark real time    |    306.8 ms |   186.5 ms | **150.0 ms** |    152.2 ms |
| CPUs utilized          |       1.024 |      2.033 |        4.165 |   **5.313** |
| IPC                    |    **1.05** |       0.87 |         0.77 |        0.86 |
| Context switches/sec   |         426 |        676 |    **9.53k** |  **39.45k** |
| CPU migrations/sec     |        1.06 |       1.87 |    **18.66** |   **82.16** |
| L1D miss rate          |   **0.08%** |      0.54% |        0.91% |       0.85% |
| LLC load rate          |    0.758M/s |   3.544M/s |     4.723M/s |    4.930M/s |
| LLC miss rate          |      53.62% | **41.42%** |       45.20% |      45.33% |
| User time              |     0.927 s |    1.557 s |      3.142 s |     2.930 s |
| System time            | **0.016 s** |    0.036 s |  **0.633 s** | **1.337 s** |
| Sys / (user+sys)       |    **1.7%** |       2.2% |    **16.8%** |   **31.3%** |


| Metric                          | 1 worker | 2 workers | 3 workers |    4 workers | 5 workers |  6 workers |
| ------------------------------- | -------: | --------: | --------: | -----------: | --------: | ---------: |
| **Throughput**                  | 325.9k/s |  536.3k/s |  625.1k/s | **666.9k/s** |  581.8k/s |   657.1k/s |
| **Speedup vs 1W**               |    1.00× |     1.65× |     1.92× |    **2.05×** |     1.79× |      2.02× |
| **Scaling efficiency**          |     100% |     82.3% |     64.0% |        51.2% |     35.7% |      33.6% |
| **CPUs utilized**               |    1.024 |     2.033 |     3.039 |        4.165 |     5.015 |      5.313 |
| **IPC**                         |     1.05 |      0.87 |      0.86 |         0.77 |      0.83 |       0.86 |
| **Context switches/sec**        |      426 |       676 |     4.29k |        9.53k |    12.38k | **39.45k** |
| **CPU migrations/sec**          |     1.06 |      1.87 |      8.58 |        18.66 |     26.91 |  **82.16** |
| **L1D miss rate**               |    0.08% |     0.54% |     0.61% |        0.91% |     0.89% |      0.85% |
| **LLC load rate**               | 0.758M/s |  3.544M/s |  3.649M/s |     4.723M/s |  4.646M/s |   4.930M/s |
| **LLC miss rate**               |   53.62% |    41.42% |    44.07% |       45.20% |    44.64% |     45.33% |
| **User time**                   |  0.927 s |   1.557 s |   2.197 s |      3.142 s |   3.582 s |    2.930 s |
| **System time**                 |  0.016 s |   0.036 s |   0.254 s |      0.633 s |   1.556 s |    1.337 s |
| **Derived kernel CPU fraction** |     1.7% |      2.2% | **10.4%** |    **16.8%** | **30.3%** |  **31.3%** |

Workers                1       2       3       4       5       6

Kernel fraction       1.7%    2.2%   10.4%   16.8%   30.3%   31.3%
Context switches/s     426     676    4.29k   9.53k   12.38k  39.45k
CPUs utilized         1.02    2.03    3.04    4.17     5.02    5.31

| Metric               | 2 workers | 4 workers |            Change |
| -------------------- | --------: | --------: | ----------------: |
| Throughput           |  536.3k/s |  666.9k/s |        **+24.4%** |
| CPUs utilized        |      2.03 |      4.17 |         **+105%** |
| IPC                  |      0.87 |      0.77 |        **−11.5%** |
| Context switches/sec |       676 |     9,531 |         **14.1×** |
| CPU migrations/sec   |      1.87 |     18.66 |           **10×** |
| L1D miss rate        |     0.54% |     0.91% |         increases |
| LLC miss rate        |    41.42% |    45.20% |   modest increase |
| Kernel CPU fraction  |     ~2.2% |    ~16.8% | **huge increase** |

