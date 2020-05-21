/*--------------------------------------------------------------------------------------------------
 *
 * testGrowingGlobalPoolAllocator.cpp
 *
 *--------------------------------------------------------------------------------------------------
 */

#include "../GrowingGlobalPoolAllocator.h"
#include "../GrowingGlobalPoolAllocator_impl.h"

#include <random>

#include <sys/sysinfo.h>
#include <sys/types.h>

#include <gtest/gtest.h>

namespace hgalloc {

struct IntAllocator : ::testing::Test {
	GrowingGlobalPoolAllocator<std::uint64_t, 10, 8> allocator{};
};

TEST_F(IntAllocator, Works)
{

	auto badger(allocator.Allocate());

	*badger = 10;
	ASSERT_EQ(*badger, 10);

	{
		auto fox(allocator.Allocate());
		*fox = 42;

		ASSERT_EQ(*fox, 42);
		ASSERT_EQ(*badger, 10);
		ASSERT_NE(fox.get(), badger.get());
	}

	ASSERT_EQ(*badger, 10);
}

TEST_F(IntAllocator, CanBeConst)
{
	const auto badger(allocator.Allocate(10));

	ASSERT_EQ(*badger.get(), 10);
	ASSERT_EQ(*badger, 10);
	ASSERT_NE(nullptr, badger);
}

TEST_F(IntAllocator, FreeReusesSameMemory)
{
	const auto lhs([this]() {
		auto lhsPtr(allocator.Allocate());
		return lhsPtr.get();
	}());

	auto rhs(allocator.Allocate().get());
	ASSERT_EQ(lhs, rhs);
}

TEST_F(IntAllocator, SizeReturnsCorrectSize)
{
	ASSERT_EQ(allocator.Size(), 0);
	{
		auto a(allocator.Allocate());
		ASSERT_EQ(allocator.Size(), 1);
		auto b(allocator.Allocate());
		ASSERT_EQ(allocator.Size(), 2);
		auto c(allocator.Allocate());
		ASSERT_EQ(allocator.Size(), 3);
		auto d(allocator.Allocate());
		ASSERT_EQ(allocator.Size(), 4);
		auto e(allocator.Allocate());

		ASSERT_EQ(allocator.Size(), 5);

		{
			auto a2(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 6);
			auto b2(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 7);
			auto c2(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 8);
			auto d2(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 9);
			auto e2(allocator.Allocate());

			ASSERT_EQ(allocator.Size(), 10);
		}

		ASSERT_EQ(allocator.Size(), 5);
	}

	ASSERT_EQ(allocator.Size(), 0);
}

TEST_F(IntAllocator, CreatingMoreThanMaxSize_ReturnsNullptr)
{
	ASSERT_EQ(allocator.Size(), 0);
	{
		auto a(allocator.Allocate());
		auto b(allocator.Allocate());
		auto c(allocator.Allocate());
		auto d(allocator.Allocate());
		auto e(allocator.Allocate());
		auto f(allocator.Allocate());
		auto g(allocator.Allocate());
		auto h(allocator.Allocate());
		auto i(allocator.Allocate());

		ASSERT_EQ(allocator.Size(), 9);
		{

			// Can create 10th
			auto j(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 10);
			ASSERT_NE(nullptr, j);
			ASSERT_NE(nullptr, j);

			// can't create 11th
			auto k(allocator.Allocate());
			ASSERT_EQ(allocator.Size(), 10);

			ASSERT_EQ(nullptr, k);
			ASSERT_EQ(nullptr, k);
		}

		// Once space reclaimed, can create again.
		ASSERT_EQ(allocator.Size(), 9);
		auto l(allocator.Allocate());
		ASSERT_EQ(allocator.Size(), 10);

		ASSERT_NE(nullptr, l);
	}

	ASSERT_EQ(allocator.Size(), 0);
}

struct LargeIntAllocator : ::testing::Test {
	using Allocator = GrowingGlobalPoolAllocator<std::uint64_t, 200, 8>;
	Allocator allocator{};
};

TEST_F(LargeIntAllocator, ReturnsCorrectValues)
{
	std::unordered_map<std::size_t, Allocator::PtrType> map;

	for (std::size_t i(0); i < allocator.Capacity(); ++i) {
		Allocator::PtrType ptr(allocator.Allocate());
		*ptr = i;
		map.insert(std::make_pair(i, std::move(ptr)));
	}

	for (auto &[value, ptr] : map) { ASSERT_EQ(value, *ptr); }
}

TEST_F(LargeIntAllocator, RandomDeletes_ReturnsCorrectly)
{

	std::mt19937 gen(100);
	std::uniform_int_distribution<> dis(0, 1);

	for (std::size_t runs(0); runs < 10; ++runs) {
		std::cerr << "Starting run " << runs << std::endl;
		std::unordered_map<std::size_t, Allocator::PtrType> map;
		for (std::size_t i(0); i < allocator.Capacity(); ++i) {
			Allocator::PtrType ptr(allocator.Allocate());
			*ptr = i;
			map.insert(std::make_pair(i, std::move(ptr)));
		}

		for (auto iter(map.begin()); iter != map.end();) {
			if (dis(gen) == 0) {
				iter = map.erase(iter);
			} else {
				++iter;
			}
		}

		for (auto &[value, ptr] : map) { ASSERT_EQ(value, *ptr); }
	}
}

std::size_t ctorsCalled(0);
std::size_t dtorsCalled(0);

struct CtorDtorCounted {
	CtorDtorCounted() { ++ctorsCalled; }

	~CtorDtorCounted() { ++dtorsCalled; }

	// Global pool only supports types >= 4 bytes
	char padding_[4];
};

struct CtorDtorCountedFixture : ::testing::Test {
	CtorDtorCountedFixture()
	{
		ctorsCalled = 0;
		dtorsCalled = 0;
	}

	GrowingGlobalPoolAllocator<CtorDtorCounted, 10, 8> allocator{};
};

TEST_F(CtorDtorCountedFixture, OnStartupDoesntCallCtor) { ASSERT_EQ(0, ctorsCalled); }

TEST_F(CtorDtorCountedFixture, CallsCtorsAndDtorsCorrectly)
{
	{
		auto a(allocator.Allocate());
		ASSERT_EQ(1, ctorsCalled);
		ASSERT_EQ(0, dtorsCalled);

		{
			auto b(allocator.Allocate());
			ASSERT_EQ(2, ctorsCalled);
			ASSERT_EQ(0, dtorsCalled);
		}

		ASSERT_EQ(2, ctorsCalled);
		ASSERT_EQ(1, dtorsCalled);

		auto c(allocator.Allocate());

		ASSERT_EQ(3, ctorsCalled);
		ASSERT_EQ(1, dtorsCalled);

		c.reset();

		ASSERT_EQ(3, ctorsCalled);
		ASSERT_EQ(2, dtorsCalled);
	}

	ASSERT_EQ(3, dtorsCalled);
	ASSERT_EQ(3, ctorsCalled);
}

struct NonDefaultConstructable {
	explicit NonDefaultConstructable(std::unique_ptr<int> var) : var_(std::move(var)) {}

	std::unique_ptr<int> var_;
};

struct NonDefaultConstructableFixture : ::testing::Test {
	GrowingGlobalPoolAllocator<NonDefaultConstructable, 10, 8> allocator{};
};

TEST_F(NonDefaultConstructableFixture, CanBeConst)
{
	const auto badger(allocator.Allocate(std::make_unique<int>(10)));

	ASSERT_EQ(*(badger->var_), 10);
	ASSERT_EQ(*(badger.get()->var_), 10);
	ASSERT_EQ(*(*badger).var_, 10);
}

TEST_F(NonDefaultConstructableFixture, WorksFine)
{
	auto badger(allocator.Allocate(std::make_unique<int>(10)));

	ASSERT_EQ(*(badger->var_), 10);

	{
		auto fox(allocator.Allocate(std::make_unique<int>(42)));

		ASSERT_EQ(*(fox->var_), 42);
		ASSERT_EQ(*(badger->var_), 10);
	}

	ASSERT_EQ(*(badger->var_), 10);
}

TEST_F(NonDefaultConstructableFixture, TypeIsMovable)
{
	{
		auto badger(allocator.Allocate(std::make_unique<int>(10)));

		ASSERT_EQ(allocator.Size(), 1);

		auto movedBadger(std::move(badger));
		ASSERT_EQ(nullptr, badger);
		ASSERT_EQ(allocator.Size(), 1);
		ASSERT_EQ(*movedBadger->var_, 10);

		{
			auto equalityMovedBadger = std::move(movedBadger);
			ASSERT_EQ(nullptr, movedBadger);
			ASSERT_EQ(*equalityMovedBadger->var_, 10);
			ASSERT_EQ(allocator.Size(), 1);

			{
				auto other = allocator.Allocate(std::make_unique<int>(20));
				ASSERT_EQ(allocator.Size(), 2);

				equalityMovedBadger = std::move(other);
				ASSERT_EQ(allocator.Size(), 1);
			}

			ASSERT_EQ(*equalityMovedBadger->var_, 20);
			ASSERT_EQ(allocator.Size(), 1);
		}
		ASSERT_EQ(allocator.Size(), 0);
	}

	ASSERT_EQ(allocator.Size(), 0);
}

std::size_t CurrentMem()
{
	struct sysinfo memInfo;

	sysinfo(&memInfo);

	std::size_t physMemUsed = memInfo.totalram - memInfo.freeram;
	//Multiply in next statement to avoid int overflow on right hand side...
	physMemUsed *= memInfo.mem_unit;

	return physMemUsed;
}

TEST(MemTest, InProgress)
{
	auto startingMem(CurrentMem());
	auto printCurMem([&]() { std::cout << CurrentMem() - startingMem << std::endl; });
	{
		using Allocator = GrowingGlobalPoolAllocator<std::array<char, 100'000>, 100'000, 16'384>;
		Allocator allocator{};
		std::vector<Allocator::PtrType> ptrs;

		printCurMem();

		for (std::size_t loop(0); loop < 10; ++loop) {

			for (std::size_t i(0); i < 100'000; ++i) {
				ptrs.push_back(allocator.Allocate());
				memcpy(ptrs.back()->data(), &i, sizeof(i));
			}

			printCurMem();

			for (std::size_t i(0); i < 100'000; ++i) { ptrs.pop_back(); }

			printCurMem();
		}

		printCurMem();
	}
	printCurMem();
}

}// namespace hgalloc
