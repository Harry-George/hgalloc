/*--------------------------------------------------------------------------------------------------
 *
 * FourByteScopedPtr.h
 *		Written for the pool allocators, we have our own FourByteScopedPtr instead of say unique_ptr as 
 *		we only need 4 bytes instead of the 8 bytes for ptr + 8 bytes for free function ptr. 
 *		Otherwise as far as the user is concerned should function exactly the same.
 * 
 *
 *--------------------------------------------------------------------------------------------------
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace hgalloc {

using FourBytePtr = std::uint32_t;

template<typename Allocator>
class FourByteScopedPtr {
public:
	explicit FourByteScopedPtr(FourBytePtr);
	static auto CreateNullPtr() -> FourByteScopedPtr;
	~FourByteScopedPtr();

	using Type = typename Allocator::Type;

	// ptr style functions
	auto operator*() -> Type &;
	auto operator*() const -> const Type &;
	auto operator->() -> Type *;
	auto operator->() const -> const Type *;
	auto reset() -> void;
	auto get() -> Type *;
	[[nodiscard]] auto get() const -> const Type *;

	// moveable
	FourByteScopedPtr(FourByteScopedPtr &&) noexcept;
	FourByteScopedPtr &operator=(FourByteScopedPtr &&) noexcept;

	// non-copyable
	FourByteScopedPtr(const FourByteScopedPtr &) = delete;
	FourByteScopedPtr &operator=(const FourByteScopedPtr &) = delete;

	static constexpr std::uint32_t NULL_PTR{std::numeric_limits<std::uint32_t>::max()};
	static constexpr std::uint32_t MAX_PTR{NULL_PTR - 1};

	template<typename U>
	//NOLINTNEXTLINE(readability-redundant-declaration) - https://github.com/cms-sw/cmssw/issues/20318
	friend bool operator==(const std::nullptr_t &, const FourByteScopedPtr<U> &rhs);
	template<typename U>
	//NOLINTNEXTLINE(readability-redundant-declaration) - https://github.com/cms-sw/cmssw/issues/20318
	friend bool operator!=(const std::nullptr_t &, const FourByteScopedPtr<U> &rhs);

private:
	FourBytePtr ptr_{NULL_PTR};
};

// Allows you to do nullptr == ptr
template<typename T>
bool operator==(const std::nullptr_t &, const FourByteScopedPtr<T> &rhs)
{
	return FourByteScopedPtr<T>::NULL_PTR == rhs.ptr_;
}

template<typename T>
bool operator!=(const std::nullptr_t &, const FourByteScopedPtr<T> &rhs)
{
	return !(nullptr == rhs);
}

template<typename Allocator>
FourByteScopedPtr<Allocator>::FourByteScopedPtr(FourBytePtr ptr) : ptr_(ptr)
{
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::CreateNullPtr() -> FourByteScopedPtr
{
	return FourByteScopedPtr{NULL_PTR};
}

template<typename Allocator>
FourByteScopedPtr<Allocator>::~FourByteScopedPtr()
{
	reset();
}

template<typename Allocator>
FourByteScopedPtr<Allocator>::FourByteScopedPtr(FourByteScopedPtr &&rhs) noexcept
{
	ptr_ = rhs.ptr_;
	rhs.ptr_ = NULL_PTR;
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::operator=(FourByteScopedPtr &&rhs) noexcept
		-> FourByteScopedPtr &
{
	if (this != &rhs) {
		reset();
		ptr_ = rhs.ptr_;
		rhs.ptr_ = NULL_PTR;
	}
	return *this;
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::operator*() -> Type &
{
	return *get();
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::operator*() const -> const Type &
{
	return *get();
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::operator->() -> Type *
{
	return get();
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::operator->() const -> const Type *
{
	return get();
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::reset() -> void
{
	if (ptr_ != NULL_PTR) {
		Allocator::Free(ptr_, get());
		ptr_ = NULL_PTR;
	}
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::get() -> Type *
{
	if (NULL_PTR == ptr_) {
		return nullptr;
	}
	// TODO: When we get it I think this stuff may need std::launder. If you understand this better
	// than me than feel free to correct this comment.
	// https://stackoverflow.com/questions/39382501/what-is-the-purpose-of-stdlaunder
	return reinterpret_cast<Type *>(&(Allocator::GetMemory(ptr_)));
}

template<typename Allocator>
auto FourByteScopedPtr<Allocator>::get() const -> const Type *
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
	return const_cast<FourByteScopedPtr *>(this)->get();
}

}// namespace hgalloc
