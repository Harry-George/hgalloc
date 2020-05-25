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
UniquePtrBM                                     7539638 ns      7528305 ns           90
GrowingGlobalPoolAllocatorBM                    2815209 ns      2813695 ns          246
UniquePtrRoundRobinBM                            437341 ns       437108 ns         1615
GrowingGlobalPoolAllocatorRoundRobinBM           429709 ns       429453 ns         1640
UniquePtrLastRecordBM                            325558 ns       325519 ns         2135
GrowingGlobalPoolAllocatorLastRecordBM           216644 ns       216607 ns         3242
UniquePtrRandomReplaceBM                          69706 ns        69743 ns         9978
GrowingGlobalPoolAllocatorRandomReplaceBM         51434 ns        51472 ns        13275
UniquePtrSequentialAccessBM                      185288 ns       185265 ns         3797
GrowingGlobalPoolAllocatorSequentialAccessBM      62620 ns        62611 ns        11151
UniquePtrRandomAccessBM                          995677 ns       995532 ns          714
GrowingGlobalPoolAllocatorRandomAccessBM        1025848 ns      1025723 ns          686
UniquePtrFreeSequentialBM                        616204 ns       616052 ns         1186
GrowingGlobalPoolAllocatorFreeSequentialBM       292139 ns       292092 ns         2397
UniquePtrFreeReverseBM                           611691 ns       611611 ns         1150
GrowingGlobalPoolAllocatorFreeReverseBM          317626 ns       317580 ns         2220


```
