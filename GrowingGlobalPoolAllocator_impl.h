/*--------------------------------------------------------------------------------------------------
 *
 * GrowingGlobalPoolAllocator_impl.h
 * 
 * 		Implementation of GrowingGlobalPoolAllocator
 *
 *--------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GrowingGlobalPoolAllocator.h"

#include <cassert>
#include <iostream>

// Assertions for testing, but have performance implications so are disabled by default.
#ifdef HGALLOC_DEBUG_ASSERTIONS
#define HGALLOC_ASSERT(x) assert(x)
#else
#define HGALLOC_ASSERT(x)
#endif

namespace hgalloc {

template<typename T, std::size_t ms, std::size_t bs>
GrowingGlobalPoolAllocator<T, ms, bs>::GrowingGlobalPoolAllocator()
{
	HGALLOC_ASSERT(globalState_.buffers_[0].empty());
}

template<typename T, std::size_t ms, std::size_t bs>
GrowingGlobalPoolAllocator<T, ms, bs>::~GrowingGlobalPoolAllocator()
{
	if (Size() > 0) {
		std::cerr << "Pool allocator of type " << typeid(T).name() << " went out of scope with "
				  << Size() << " elements still allocated";
		HGALLOC_ASSERT(false);
	}

	// Reset the global state
	globalState_ = GlobalState{};
}

template<std::size_t number>
constexpr auto MostSignificantBitLocation() -> std::size_t
{
	std::size_t location(0);

	std::size_t i(number);
	while (i > 0) {
		i >>= 1;
		location += 1;
	}
	return location;
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::GetMemory(FourBytePtr ptr) -> MemBlock &
{
	const std::size_t bucketNum(ptr >> MostSignificantBitLocation<BUCKET_MASK>());
	const std::size_t index(ptr & BUCKET_MASK);

	return globalState_.buffers_[bucketNum][index];
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::GetMemoryOrAlloc(FourBytePtr ptr) -> MemBlock &
{
	const std::size_t bucketNum(ptr >> MostSignificantBitLocation<BUCKET_MASK>());

	auto &buffers(globalState_.buffers_);

	if (buffers[bucketNum].empty()) { buffers[bucketNum].resize(bs); }

	//	return GetMemory(ptr);

	const auto index(ptr & BUCKET_MASK);
	HGALLOC_ASSERT(buffers[bucketNum].size() > index);

	return buffers[bucketNum][index];
}

template<typename T, std::size_t ms, std::size_t bs>
template<typename... Args>
auto GrowingGlobalPoolAllocator<T, ms, bs>::Allocate(Args &&... args) -> PtrType
{
	if (globalState_.totalFreeListSize_ > 0) {
		// First we check to see if we have any previously freed elements and will use them first
		auto [block, ptr](PopFreeList());
		new (&block) T(std::forward<Args>(args)...);// emplace onto our buffer
		return PtrType{ptr};
	}

	// Failing that, find the index into the next element in the vector
	const std::uint32_t nextIndex(globalState_.numOfElements_);

	if (nextIndex < ms) {
		// We have space, so add element in vector and return that.
		++globalState_.numOfElements_;
		new (&GetMemoryOrAlloc(nextIndex)) T(std::forward<Args>(args)...);// emplace onto our buffer
		return PtrType{nextIndex};
	}

	return PtrType::CreateNullPtr();
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::Free(FourBytePtr ptr, T *value) -> void
{
	if (value == nullptr) { return; }

	value->~T();

	// Push our record on the front of the free list
	PushFreeList(ptr);

	// We don't want to close the second we cross over a threshold
	constexpr std::size_t numOfFreeElementsBeforeEviction(bs + (bs / 2));

	static std::size_t freeCount(0);
	freeCount++;
	if (freeCount == bs) {
		freeCount = 0;

		if (globalState_.totalFreeListSize_ > numOfFreeElementsBeforeEviction) {
			const std::size_t highestInsertedPointer(globalState_.numOfElements_ - 1);
			const std::size_t highestBucket(highestInsertedPointer >>
											MostSignificantBitLocation<BUCKET_MASK>());
			HGALLOC_ASSERT(highestBucket < NUM_OF_BUCKETS);


			const std::size_t highestIndexInBucket(highestInsertedPointer & BUCKET_MASK);
			const std::size_t bucketSize(highestIndexInBucket + 1);

			auto &freeList(globalState_.freeLists_[highestBucket]);
			if (freeList.freeListSize_ == bucketSize) {
				// we can evict an entire frame
				freeList.freeListSize_ = 0;
				freeList.freeList_ = PtrType::NULL_PTR;
				globalState_.totalFreeListSize_ -= bucketSize;
				globalState_.numOfElements_ -= bucketSize;
				// From http://www.cplusplus.com/reference/vector/vector/clear/ as a way to force
				// reallocation
				std::vector<MemBlock>().swap(globalState_.buffers_[highestBucket]);
			}
		}
	}
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::Size() const -> std::size_t
{
	return globalState_.numOfElements_ - globalState_.totalFreeListSize_;
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::PopFreeList() -> BlockAndPtr
{
	auto &freeLists(globalState_.freeLists_);
	for (std::size_t i(globalState_.smallestBucket_); i < NUM_OF_BUCKETS; ++i) {
		auto &freeList(freeLists[i]);
		if (freeList.freeList_ != PtrType::NULL_PTR) {
			HGALLOC_ASSERT(freeList.freeListSize_ > 0);

			const FourBytePtr nextElement(freeList.freeList_);
			MemBlock &element(GetMemory(nextElement));
			freeList.freeList_ = *reinterpret_cast<FourBytePtr *>(&element);

			--freeList.freeListSize_;
			globalState_.smallestBucket_ = i;
			--globalState_.totalFreeListSize_;

			return {element, nextElement};
		}
		HGALLOC_ASSERT(freeList.freeListSize_ == 0);
	}

	assert(false);
}

template<typename T, std::size_t ms, std::size_t bs>
auto GrowingGlobalPoolAllocator<T, ms, bs>::PushFreeList(FourBytePtr ptr) -> void
{
	HGALLOC_ASSERT(ptr != PtrType::NULL_PTR);
	const std::size_t bucketNum(ptr >> MostSignificantBitLocation<BUCKET_MASK>());
	HGALLOC_ASSERT(bucketNum < NUM_OF_BUCKETS);

	auto &freeList(globalState_.freeLists_[bucketNum]);

	*reinterpret_cast<FourBytePtr *>(&GetMemory(ptr)) = freeList.freeList_;
	freeList.freeList_ = ptr;

	++globalState_.totalFreeListSize_;
	++freeList.freeListSize_;

	globalState_.smallestBucket_ = std::min(globalState_.smallestBucket_, bucketNum);
}


}// namespace hgalloc
