/*--------------------------------------------------------------------------------------------------
 *
 * test/perfGrowingGlobalPoolAllocator.cpp
 *
 *--------------------------------------------------------------------------------------------------
 */

#include <benchmark/benchmark.h>

#include <memory>

#include "../GrowingGlobalPoolAllocator_impl.h"
#include <random>
#include <valgrind/callgrind.h>
#include <vector>

namespace hgalloc {

struct BigType {
	char var_[200];
};

std::size_t runSize{100'000};

void UniquePtrBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<BigType>> ret;
	ret.reserve(runSize);

	for (auto _ : state) {
		for (std::size_t i(0); i < runSize; ++i) { ret.push_back(std::make_unique<BigType>()); }
		ret.clear();
	}
}
BENCHMARK(UniquePtrBM);

void GrowingGlobalPoolAllocatorBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<BigType, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);

	for (auto _ : state) {
		for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate()); }
		ret.clear();
	}
}
BENCHMARK(GrowingGlobalPoolAllocatorBM);

void UniquePtrRoundRobinBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<BigType>> ret;
	ret.reserve(runSize);
	auto perRun(runSize / 10);

	// Fill Vector
	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(std::make_unique<BigType>()); }

	for (auto _ : state) {
		ret.erase(ret.begin(), ret.begin() + perRun);
		for (std::size_t i(0); i < perRun; ++i) { ret.push_back(std::make_unique<BigType>()); }
	}
}
BENCHMARK(UniquePtrRoundRobinBM);

void GrowingGlobalPoolAllocatorRoundRobinBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<BigType, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);
	auto perRun(runSize / 10);

	// Fill Vector
	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate()); }

	for (auto _ : state) {
		CALLGRIND_START_INSTRUMENTATION;
		ret.erase(ret.begin(), ret.begin() + perRun);
		for (std::size_t i(0); i < perRun; ++i) { ret.push_back(allocator.Allocate()); }
		CALLGRIND_STOP_INSTRUMENTATION;
	}
}

BENCHMARK(GrowingGlobalPoolAllocatorRoundRobinBM);

void UniquePtrLastRecordBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<BigType>> ret;
	ret.reserve(runSize);
	auto perRun(runSize / 10);

	// Fill Vector
	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(std::make_unique<BigType>()); }

	for (auto _ : state) {
		ret.erase(ret.end() - perRun, ret.end());
		for (std::size_t i(0); i < perRun; ++i) { ret.push_back(std::make_unique<BigType>()); }
	}
}
BENCHMARK(UniquePtrLastRecordBM);

void GrowingGlobalPoolAllocatorLastRecordBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<BigType, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);
	auto perRun(runSize / 10);

	// Fill Vector
	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate()); }

	for (auto _ : state) {
		ret.erase(ret.end() - perRun, ret.end());
		for (std::size_t i(0); i < perRun; ++i) { ret.push_back(allocator.Allocate()); }
	}
}
BENCHMARK(GrowingGlobalPoolAllocatorLastRecordBM);

std::size_t numRandomDeletes(1'000);

void UniquePtrRandomReplaceBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<BigType>> ret;
	ret.reserve(runSize);

	// Fill Vector
	for (std::size_t i(0); i < runSize - 1; ++i) { ret.push_back(std::make_unique<BigType>()); }

	std::mt19937 gen(100);
	std::uniform_int_distribution<> dis(0, ret.size() - 1);

	std::vector<std::size_t> randomLocations;
	randomLocations.resize(numRandomDeletes);

	for (auto _ : state) {
		state.PauseTiming();// Stop timers. They will not count until they are resumed.
		for (std::size_t i(0); i < numRandomDeletes; ++i) { randomLocations[i] = dis(gen); }
		state.ResumeTiming();

		for (const auto location : randomLocations) { ret[location].reset(); }

		for (const auto location : randomLocations) { ret[location] = std::make_unique<BigType>(); }
	}
}
BENCHMARK(UniquePtrRandomReplaceBM);

void GrowingGlobalPoolAllocatorRandomReplaceBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<BigType, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);

	// Fill Vector
	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate()); }

	std::mt19937 gen(100);
	std::uniform_int_distribution<> dis(0, ret.size() - 1);

	std::vector<std::size_t> randomLocations;
	randomLocations.resize(numRandomDeletes);

	for (auto _ : state) {
		state.PauseTiming();// Stop timers. They will not count until they are resumed.
		for (std::size_t i(0); i < numRandomDeletes; ++i) { randomLocations[i] = dis(gen); }
		state.ResumeTiming();

		for (const auto location : randomLocations) { ret[location].reset(); }

		for (const auto location : randomLocations) { ret[location] = allocator.Allocate(); }
	}
}
BENCHMARK(GrowingGlobalPoolAllocatorRandomReplaceBM);

void UniquePtrSequentialAccessBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<int>> ret;
	ret.reserve(runSize);

	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(std::make_unique<int>(i)); }
	for (auto _ : state) {
		for (const auto &var : ret) { benchmark::DoNotOptimize(*var); }
	}
}
BENCHMARK(UniquePtrSequentialAccessBM);

void GrowingGlobalPoolAllocatorSequentialAccessBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<int, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);

	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate(i)); }
	for (auto _ : state) {
		for (const auto &var : ret) { benchmark::DoNotOptimize(*var); }
	}
}
BENCHMARK(GrowingGlobalPoolAllocatorSequentialAccessBM);

void UniquePtrRandomAccessBM(benchmark::State &state)
{
	std::vector<std::unique_ptr<int>> ret;
	ret.reserve(runSize);

	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(std::make_unique<int>(i)); }

	std::mt19937 gen(100);
	std::uniform_int_distribution<> dis(0, ret.size() - 1);

	std::vector<std::size_t> randomLocations;
	randomLocations.resize(numRandomDeletes);
	for (std::size_t i(0); i < runSize; ++i) { randomLocations.push_back(dis(gen)); }

	for (auto _ : state) {
		for (const auto &var : randomLocations) { benchmark::DoNotOptimize(*ret[var]); }
	}
}
BENCHMARK(UniquePtrRandomAccessBM);

void GrowingGlobalPoolAllocatorRandomAccessBM(benchmark::State &state)
{
	using Allocator = GrowingGlobalPoolAllocator<int, 100'000, 16'384>;
	Allocator allocator{};
	std::vector<Allocator::PtrType> ret;
	ret.reserve(runSize);

	for (std::size_t i(0); i < runSize; ++i) { ret.push_back(allocator.Allocate(i)); }
	std::mt19937 gen(100);
	std::uniform_int_distribution<> dis(0, ret.size() - 1);

	std::vector<std::size_t> randomLocations;
	randomLocations.resize(numRandomDeletes);
	for (std::size_t i(0); i < runSize; ++i) { randomLocations.push_back(dis(gen)); }

	for (auto _ : state) {
		for (const auto &var : randomLocations) { benchmark::DoNotOptimize(*ret[var]); }
	}
}
BENCHMARK(GrowingGlobalPoolAllocatorRandomAccessBM);

}// namespace hgalloc

BENCHMARK_MAIN();
