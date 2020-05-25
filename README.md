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
UniquePtrBM                                    18946129 ns     18923832 ns           37
GrowingGlobalPoolAllocatorBM                   11774704 ns     11770881 ns           58
UniquePtrRoundRobinBM                           5528195 ns      5527229 ns          126
GrowingGlobalPoolAllocatorRoundRobinBM          1605000 ns      1604643 ns          439
UniquePtrLastRecordBM                           1462836 ns      1462579 ns          468
GrowingGlobalPoolAllocatorLastRecordBM          1124818 ns      1124668 ns          623
UniquePtrRandomReplaceBM                         180740 ns       180743 ns         3830
GrowingGlobalPoolAllocatorRandomReplaceBM        147821 ns       147838 ns         4695
UniquePtrSequentialAccessBM                     1381399 ns      1381169 ns          509
GrowingGlobalPoolAllocatorSequentialAccessBM    2701806 ns      2701443 ns          327
UniquePtrRandomAccessBM                         2044280 ns      2044001 ns          343
GrowingGlobalPoolAllocatorRandomAccessBM        2796550 ns      2796062 ns          252
UniquePtrFreeSequentialBM                       3925594 ns      3924985 ns          172
GrowingGlobalPoolAllocatorFreeSequentialBM      5124077 ns      5123418 ns          137
UniquePtrFreeReverseBM                          2980278 ns      2979809 ns          247
GrowingGlobalPoolAllocatorFreeReverseBM         5470640 ns      5469816 ns          129

```
