/*--------------------------------------------------------------------------------------------------
 *
 * testFourByteScopedPtr.cpp
 *
 *--------------------------------------------------------------------------------------------------
 */

#include "../FourByteScopedPtr.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace hgalloc {

using ::testing::_;


template<class T>
class StaticMock {
public:
	StaticMock() { *GetPtrPtr() = static_cast<T *>(this); }

	~StaticMock() { *GetPtrPtr() = nullptr; }

	static T &GetInstance()
	{
		assert(*GetPtrPtr());
		return **GetPtrPtr();
	}

private:
	static T **GetPtrPtr()
	{
		static T *instance(nullptr);
		return &instance;
	}
};


struct MockAllocator : StaticMock<MockAllocator> {
	using Type = std::string;

	MOCK_CONST_METHOD0(GetMemoryMock, std::vector<Type> &());
	MOCK_CONST_METHOD2(FreeMock, void(FourBytePtr, Type *));

	static auto GetMemory(FourBytePtr ptr) -> Type &
	{
		return MockAllocator::GetInstance().GetMemoryMock()[ptr];
	}

	static auto Free(FourBytePtr ptr, Type *type) -> void
	{
		MockAllocator::GetInstance().FreeMock(ptr, type);
	}
};

struct Fixture : ::testing::Test {
	::testing::NiceMock<MockAllocator> allocator;
	using Ptr = FourByteScopedPtr<MockAllocator>;
};

TEST_F(Fixture, NullPtrEqualsNullPtr)
{
	EXPECT_CALL(allocator, FreeMock(_, _)).Times(0);
	ASSERT_TRUE(nullptr == Ptr::CreateNullPtr());
	ASSERT_FALSE(nullptr != Ptr::CreateNullPtr());
	ASSERT_TRUE(nullptr == Ptr::CreateNullPtr().get());
	ASSERT_FALSE(nullptr != Ptr::CreateNullPtr().get());
}

struct BufferOfStrings : Fixture {
	std::vector<std::string> strings = {"String1", "String2"};

	BufferOfStrings()
	{
		ON_CALL(allocator, GetMemoryMock()).WillByDefault(::testing::ReturnRef(strings));
	}
};

TEST_F(BufferOfStrings, AccessorsWork)
{
	Ptr a(0);
	const Ptr b(1);
	ASSERT_EQ(*a, "String1");
	ASSERT_EQ(*b, "String2");

	ASSERT_EQ(std::string(a->data()), "String1");
	ASSERT_EQ(std::string(b->data()), "String2");

	ASSERT_EQ(*(a.get()), "String1");
	ASSERT_EQ(*(b.get()), "String2");
}

TEST_F(BufferOfStrings, AccessorsCanChangeUnderlyingData)
{
	{
		Ptr a(0);
		ASSERT_EQ(*a, "String1");
		*a = "String2";
	}

	{
		Ptr a(0);
		ASSERT_EQ(*a, "String2");
		a->clear();
	}

	{
		Ptr a(0);
		ASSERT_EQ(*a, "");
		*(a.get()) = "Badger";
	}

	Ptr a(0);
	ASSERT_EQ(*a, "Badger");
}

TEST_F(BufferOfStrings, GoesOutOfScope_CallsFree)
{
	const auto index(0);
	EXPECT_CALL(allocator, FreeMock(index, &strings[index])).Times(1);
	{
		Ptr a(index);
	}
}

TEST_F(BufferOfStrings, Reset_CallsFreeOnce)
{
	const auto index(0);
	EXPECT_CALL(allocator, FreeMock(index, &strings[index])).Times(1);
	{
		Ptr a(index);
		a.reset();
		ASSERT_EQ(nullptr, a);
	}
}

}// namespace hgalloc
