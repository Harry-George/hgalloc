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
UniquePtrBM                                     8289114 ns      8282727 ns           86
GrowingGlobalPoolAllocatorBM                    3014760 ns      3013735 ns          199
UniquePtrRoundRobinBM                            421130 ns       421004 ns         1666
GrowingGlobalPoolAllocatorRoundRobinBM           453376 ns       453232 ns         1560
UniquePtrLastRecordBM                            337667 ns       337611 ns         2041
GrowingGlobalPoolAllocatorLastRecordBM           233814 ns       233774 ns         2978
UniquePtrRandomReplaceBM                          72292 ns        72327 ns         9446
GrowingGlobalPoolAllocatorRandomReplaceBM         54016 ns        54041 ns        12738
UniquePtrSequentialAccessBM                      194596 ns       194558 ns         3615
GrowingGlobalPoolAllocatorSequentialAccessBM      64589 ns        64579 ns        10842
UniquePtrRandomAccessBM                         1026623 ns      1026462 ns          682
GrowingGlobalPoolAllocatorRandomAccessBM        1069540 ns      1069298 ns          652
UniquePtrFreeSequentialBM                        611618 ns       611522 ns         1144
GrowingGlobalPoolAllocatorFreeSequentialBM       408857 ns       408785 ns         1713
UniquePtrFreeReverseBM                           631077 ns       630986 ns         1123
GrowingGlobalPoolAllocatorFreeReverseBM          327638 ns       327574 ns         2139


```
