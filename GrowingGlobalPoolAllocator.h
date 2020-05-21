/*--------------------------------------------------------------------------------------------------
 *
 * GrowingGlobalPoolAllocator.h
 *		Allocator for storing lots of a certain type of object. 
 *
 *--------------------------------------------------------------------------------------------------
 */

#pragma once

#include "FourByteScopedPtr.h"

#include <array>
#include <limits>
#include <memory>
#include <stack>
#include <vector>

namespace hgalloc {

template<
		// The type to store
		typename T,
		// the max number of elements for the global allocator to store. So for max size its
		// sizeof(T) * maxElements
		std::size_t maxElements,
		std::size_t bucketSize>// the size of each bucket. Must be a power of 2
class GrowingGlobalPoolAllocator {
public:
	using Type = T;
	using PtrType = FourByteScopedPtr<GrowingGlobalPoolAllocator<T, maxElements, bucketSize>>;
	friend PtrType;

	static_assert(maxElements < PtrType::NULL_PTR, "Cannot store that many pointer objects");
	static_assert(maxElements > 0, "maxElements cannot be zero");
	static_assert(bucketSize > 0, "bucketSize cannot be zero");
	static_assert(maxElements >= bucketSize,
				  "maxElements must be greater than or equal to bucket size");

//	// TODO - this properly so bucket size can be > 128
//	static_assert(bucketSize == 0b00000001 || bucketSize == 0b00000010 ||
//						  bucketSize == 0b00000100 || bucketSize == 0b00001000 ||
//						  bucketSize == 0b00010000 || bucketSize == 0b00100000 ||
//						  bucketSize == 0b01000000 || bucketSize == 0b10000000,
//				  "Bucket Size must be a power of 2");


	// not-movable
	GrowingGlobalPoolAllocator(GrowingGlobalPoolAllocator &&) = delete;
	GrowingGlobalPoolAllocator &operator=(GrowingGlobalPoolAllocator &&) = delete;
	// non-copyable
	GrowingGlobalPoolAllocator(const GrowingGlobalPoolAllocator &) = delete;
	GrowingGlobalPoolAllocator &operator=(const GrowingGlobalPoolAllocator &) = delete;

	explicit GrowingGlobalPoolAllocator();
	~GrowingGlobalPoolAllocator();

	// Returns a unique ptr like object that will free its memory when it exits scope
	template<typename... Args>
	auto Allocate(Args &&...) -> PtrType;

	[[nodiscard]] auto Size() const -> std::size_t;
	[[nodiscard]] constexpr auto Capacity() const -> std::size_t { return maxElements; }

private:
	static auto Free(FourBytePtr, T *) -> void;

	// We use a memblock so we can allocate types that are not default constructable
	struct MemBlock {
		std::array<char, sizeof(T)> buf;
	};

	static_assert(sizeof(MemBlock) == sizeof(T), "Currently doesn't support packed types");
	static_assert(sizeof(T) >= sizeof(FourBytePtr),
				  "We need the size of the object to be at least 4 bytes");

	constexpr static std::size_t NUM_OF_BUCKETS{(maxElements / bucketSize) +
												(maxElements % bucketSize == 0 ? 0 : 1)};
	constexpr static std::size_t BUCKET_MASK{bucketSize - 1};

	struct FreeList {
		// We create a linked list of free memory, this means we don't need any extra memory
		// for our free list and freeing can't throw.
		FourBytePtr freeList_{PtrType::NULL_PTR};
		std::size_t freeListSize_{0};
	};

	struct GlobalState {
		std::array<std::vector<MemBlock>, NUM_OF_BUCKETS> buffers_{{}};
		// We create a linked list of free memory, this means we don't need any extra memory
		// for our free list and freeing can't throw.
		std::array<FreeList, NUM_OF_BUCKETS> freeLists_{{}};
		// We store an extra bit of information here to stop us having to read
		// all the free list sizes each allocation
		std::size_t totalFreeListSize_{0};
		std::size_t numOfElements_{0};
		std::size_t smallestBucket_{0};
	};

	// Accessors to static internal state. Makes the lifetime much easier to manage.
	static auto GetGlobalState() -> GlobalState &;

	// Convenience accessors to global state members
	static auto Buffers() -> std::array<std::vector<MemBlock>, NUM_OF_BUCKETS> &;
	struct BlockAndPtr {
		MemBlock &memBlock;
		FourBytePtr ptr;
	};
	static auto PopFreeList() -> BlockAndPtr;
	static auto PushFreeList(FourBytePtr) -> void;
	static auto FreeListSize() -> std::size_t &;
	static auto NumOfElements() -> std::size_t &;
	static auto GetMemory(FourBytePtr ptr) -> MemBlock &;
	static auto GetMemoryOrAlloc(FourBytePtr ptr) -> MemBlock &;
};

}// namespace hgalloc
