#define _CRT_SECURE_NO_WARNINGS

#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdint>
#include <vector>

#include <Windows.h>

#include "frameData.h"
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

	benchStruct benchData(dim, taps);
	//benchStruct benchData(dim, taps, std::thread::hardware_concurrency());

	benchData.setIOBuffers(in.data(), in.data(), out.data(), out.data());

	uint8_t* temp = new uint8_t[dim * dim * 3];

	SetProcessAffinityMask(GetCurrentProcess(), 0x30);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	
	// Main timing loop
	for (auto _ : s) {
		//s.PauseTiming();
		//std::memcpy(temp, in.data(), dim * dim * 3);
		//s.ResumeTiming();
		//benchData.processFrame(temp, out.data());

		benchData.fakeWriteInput();
		benchData.fakeReadOutput();
	}

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	delete[] temp;

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
	uint8_t* image = new uint8_t[dim * dim * 3 * 2];
	fread(image, dim * dim * 3, 1, file_ptr);
	fclose(file_ptr);

	// Variable for our results
	AlignedVector<T> out(dim * dim * 2 * 3);

	auto benchData = new benchStruct(dim, taps);

	benchData->setIOBuffers(image, image, out.data(), out.data());

	uint8_t* temp = new uint8_t[dim * dim * 3 * 2];

	SetProcessAffinityMask(GetCurrentProcess(), 0x30);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	//// Main timing loop
	//for (auto _ : s) {
	//	s.PauseTiming();
	//	std::memcpy(temp, image, dim * dim * 3);
	//	s.ResumeTiming();
	//	benchData.processFrame(temp, out.data());
	//}

	benchData->fakeWriteInput();
	benchData->fakeReadOutput();

	delete benchData;

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	delete[] temp;

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

	delete[] image;
	return;
}

#define FILEARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), { 10 } } )->MeasureProcessCPUTime()
//#define TESTARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), { 7 } } )->MeasureProcessCPUTime()
//#define TESTARGS ArgsProduct({ benchmark::CreateRange(1, 16, 2), benchmark::CreateDenseRange(0, 10, 1) } )->MeasureProcessCPUTime()

int main(int argc, char** argv) {

	benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, true>>), &(argv[1]))->FILEARGS;

	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, true>>))->TESTARGS;

	//these entries are from BENCHMARK_MAIN
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();

}