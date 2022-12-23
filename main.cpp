#define _CRT_SECURE_NO_WARNINGS

#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdint>
#include <vector>

#include <Windows.h>

#include "frameData.h"
#include "frameDataCustom.h"
#include "AlignedVector.h"

using namespace std::chrono;

#define EXPAND_TEMPLATE_BENCH(...) #__VA_ARGS__, __VA_ARGS__

template <class benchStruct>
static void bench(benchmark::State& s) {

	using T = typename benchStruct::DataType;

	// Taps
	auto taps = s.range(0);

	// Number of elements
	auto dim = 8 << s.range(1);

	// Create a vector of random numbers
	std::vector<T> in(dim * dim * 3);
	std::generate(begin(in), end(in), [] { return rand() % 255; });

	// Variable for our results
	AlignedVector<T> out(dim * dim * 2 * 3);

	benchStruct::expandUV(in.data() + dim * dim, dim);

	{
		//benchStruct benchData(dim, taps, 1);
		benchStruct benchData(dim, taps);
		//benchStruct benchData(dim, taps, &centripetalCatMullRomInterpolation);

		benchData.setIOBuffers(in.data(), in.data(), out.data(), out.data());

		std::vector<T> temp(dim * dim * 3 * 2);

		//SetProcessAffinityMask(GetCurrentProcess(), 0x30);
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

		// Main timing loop
		for (auto _ : s) {
			benchData.fakeWriteInput();
			benchData.fakeReadOutput();
		}

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	}

	s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2));
	s.SetItemsProcessed(s.iterations());
	s.SetLabel([](int size) {
		size_t dim = size * size * 2;
		if (dim < (1 << 10))
			return std::to_string(dim) + " pixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		if (dim < (1 << 20))
			return std::to_string(dim >> 10) + " kpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		if (dim < (1 << 30))
			return std::to_string(dim >> 20) + " Mpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		return std::string("Empty");
	} (dim));

}

template <class benchStruct>
static void bench_file(benchmark::State& s, char** input) {

	using T = typename benchStruct::DataType;

	// Taps
	auto taps = s.range(0);

	std::string path = input[0];
	std::string file = input[1];

	//printf("%s\n", (path + file).c_str());

	int dim;
	sscanf(strchr(file.c_str(), 'x'), "x%d#", &dim);

	//dont need this to start making values
	FILE* file_ptr = fopen((path + file).c_str(), "rb");
	if (!file_ptr) {
		s.SkipWithError("Error reading input file");
		return;
	}

	std::vector<T> image(dim * dim * 3 * 2);
	fread(image.data(), dim * dim * 3, 1, file_ptr);
	fclose(file_ptr);

	benchStruct::expandUV(image.data() + dim * dim, dim);

	// Variable for our results
	AlignedVector<T> out(dim * dim * 2 * 3); 
	
	{
		//benchStruct benchData(dim, taps, 1);
		//benchStruct benchData(dim, taps);
		benchStruct benchData(dim, taps, &centripetalCatMullRomInterpolation);

		benchData.setIOBuffers(image.data(), image.data(), out.data(), out.data());

		std::vector<T> temp(dim * dim * 3 * 2);

		//SetProcessAffinityMask(GetCurrentProcess(), 0x30);
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

		// Main timing loop
		for (auto _ : s) {
			benchData.fakeWriteInput();
			benchData.fakeReadOutput();
		}

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	}

	s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2));
	s.SetItemsProcessed(s.iterations());
	s.SetLabel([](int size) {
		size_t dim = size * size * 2;
		if (dim < (1 << 10))
			return std::to_string(dim) + " pixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		if (dim < (1 << 20))
			return std::to_string(dim >> 10) + " kpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		if (dim < (1 << 30))
			return std::to_string(dim >> 20) + " Mpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
		return std::string("Empty");
	} (dim));

	char image_path[10240];
	(void)sprintf(image_path, "%s%s_decomp_simd_threaded.yuv", path.c_str(), strtok(const_cast<char*>(file.c_str()), "#"));
	file_ptr = fopen(image_path, "wb");
	fwrite(out.data(), 1, dim * dim * 2 * 3, file_ptr);
	fclose(file_ptr);

	return;
}

#define FILEARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), { 10 } } )->UseRealTime()
//#define TESTARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), { 7 } } )->UseRealTime()
#define TESTARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), benchmark::CreateDenseRange(7, 10, 1) } )->UseRealTime()

int main(int argc, char** argv) {

	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameDataCustom<uint8_t>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t>>), &(argv[1]))->FILEARGS;

	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameDataCustom<uint8_t>>))->TESTARGS;
	benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t>>))->TESTARGS;

	//these entries are from BENCHMARK_MAIN
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();

}