# hgalloc

This is a pool allocator that can both grow and shrink. 

It works by having a series of buckets. Each a contiguous block of <bucketSize> elements. If we need
more memory we create a new bucket. Each bucket has its own free list and we prefer to pick elements
from the lowest free list, so if you start using less elements eventually the higher buckets will
become empty. When this happens they are deleted freeing up the memory.

It returns 4 byte pointers, which behave just like a unique_ptr but are only 4 bytes large. 

Latest perf results

```
---------------------------------------------------------------------------------------
Benchmark                                             Time             CPU   Iterations
---------------------------------------------------------------------------------------
UniquePtrBM                                     7833406 ns      7823116 ns           92
GrowingGlobalPoolAllocatorBM                    3063056 ns      3061850 ns          208
UniquePtrRoundRobinBM                            460632 ns       460470 ns         1377
GrowingGlobalPoolAllocatorRoundRobinBM           428190 ns       428051 ns         1629
UniquePtrLastRecordBM                            328197 ns       328148 ns         2132
GrowingGlobalPoolAllocatorLastRecordBM           209751 ns       209716 ns         3360
UniquePtrRandomReplaceBM                          73981 ns        74014 ns         9480
GrowingGlobalPoolAllocatorRandomReplaceBM         53377 ns        53406 ns        12959
UniquePtrSequentialAccessBM                       54360 ns        54353 ns        12881
GrowingGlobalPoolAllocatorSequentialAccessBM      51190 ns        51184 ns        13653
UniquePtrRandomAccessBM                         1031426 ns      1031314 ns          684
GrowingGlobalPoolAllocatorRandomAccessBM        1024312 ns      1024174 ns          698
UniquePtrFreeSequentialBM                        607300 ns       607221 ns         1153
GrowingGlobalPoolAllocatorFreeSequentialBM       251072 ns       251044 ns         2790
UniquePtrFreeReverseBM                           624183 ns       624115 ns         1110
GrowingGlobalPoolAllocatorFreeReverseBM          264221 ns       264188 ns         2649

```
