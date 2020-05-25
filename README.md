# hgalloc

This is a pool allocator that can both grow and shrink. 

It works by having a series of buckets. Each a contiguous block of <bucketSize> elements. If we need
more memory we create a new bucket. Each bucket has its own free list and we prefer to pick elements
from the lowest free list, so if you start using less elements eventually the higher buckets will
become empty. When this happens they are deleted freeing up the memory.

It returns 4 byte pointers, which behave just like a unique_ptr but are only 4 bytes large. 
