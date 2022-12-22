#define _CRT_SECURE_NO_WARNINGS

#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdint>
#include <vector>

#include <Windows.h>

#include "frameData.h"
#include "AlignedVector.h"

#define EXPAND_TEMPLATE_BENCH(...) #__VA_ARGS__, __VA_ARGS__

template <class benchStruct>
static void bench(benchmark::State& s) {

	using T = typename benchStruct::DataType;

	// Number of elements
	auto N = 8 << s.range(0);

	// Create a vector of random numbers
	std::vector<T> in(N * N * 3);
	std::generate(begin(in), end(in), [] { return rand() % 255; });

	// Variable for our results
	AlignedVector<T> out(N * N * 2 * 3);

	benchStruct benchData(N);

	uint8_t* temp = new uint8_t[N * N * 3];

	SetProcessAffinityMask(GetCurrentProcess(), 0x30);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	// Main timing loop
	for (auto _ : s) {
		s.PauseTiming();
		std::memcpy(temp, in.data(), N * N * 3);
		s.ResumeTiming();
		benchData.processFrame(temp, out.data());
	}

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	delete[] temp;

	s.SetBytesProcessed(uint64_t(s.iterations() * N * N * 2));
	s.SetItemsProcessed(s.iterations());
	s.SetLabel([](int M) {
		size_t N = M * M * 2;
		if (N < (1 << 10))
			return std::to_string(N) + " pixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		if (N < (1 << 20))
			return std::to_string(N >> 10) + " kpixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		if (N < (1 << 30))
			return std::to_string(N >> 20) + " Mpixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		return std::string("Empty");
	} (N));

}

template <class benchStruct>
static void bench_file(benchmark::State& s, char** input) {

	using T = typename benchStruct::DataType;

	std::string path = input[0];
	std::string file = input[1];

	//printf("%s\n", (path + file).c_str());

	int N;
	sscanf(strchr(file.c_str(), 'x'), "x%d#", &N);

	//dont need this to start making values
	FILE* file_ptr = fopen((path + file).c_str(), "rb");
	if (!file_ptr) {
		s.SkipWithError("Error reading input file");
		return;
	}
	uint8_t* image = new uint8_t[N * N * 3 * 2];
	fread(image, N * N * 3, 1, file_ptr);
	fclose(file_ptr);

	// Variable for our results
	AlignedVector<T> out(N * N * 2 * 3);

	benchStruct benchData(N);

	uint8_t* temp = new uint8_t[N * N * 3 * 2];

	SetProcessAffinityMask(GetCurrentProcess(), 0x30);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	// Main timing loop
	for (auto _ : s) {
		s.PauseTiming();
		std::memcpy(temp, image, N * N * 3);
		s.ResumeTiming();
		benchData.processFrame(temp, out.data());
	}

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	delete[] temp;

	s.SetBytesProcessed(uint64_t(s.iterations() * N * N * 2));
	s.SetItemsProcessed(s.iterations());
	s.SetLabel([](int M) {
		size_t N = M * M * 2;
		if (N < (1 << 10))
			return std::to_string(N) + " pixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		if (N < (1 << 20))
			return std::to_string(N >> 10) + " kpixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		if (N < (1 << 30))
			return std::to_string(N >> 20) + " Mpixels (" + std::to_string(M * 2) + " * " + std::to_string(M) + ")";
		return std::string("Empty");
	} (N));

	char image_path[10240];
	(void)sprintf(image_path, "%s%s_decomp_simd.yuv", path.c_str(), strtok(const_cast<char*>(file.c_str()), "#"));
	file_ptr = fopen(image_path, "wb");
	fwrite(out.data(), 1, N * N * 2 * 3, file_ptr);
	fclose(file_ptr);

	delete[] image;
	return;
}

#define FILEARGS Args({ 8 })
#define TESTARGS Args({ 9 })
//#define TESTARGS DenseRange(0, 10)

int main(int argc, char** argv) {

	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 1, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 1, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 1, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 1, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 2, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 2, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 2, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 2, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 4, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 4, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 4, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 4, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 8, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 8, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 8, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 8, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 16, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, float, 16, true>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 16, false>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<uint8_t, int, 16, true>>), &(argv[1]))->FILEARGS;

	benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 1, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 1, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 1, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 1, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 2, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 2, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 2, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 2, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 4, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 4, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 4, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 4, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 8, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 8, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 8, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 8, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 16, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, float, 16, true>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 16, false>>))->TESTARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench<frameData<uint8_t, int, 16, true>>))->TESTARGS;

	//these entries are from BENCHMARK_MAIN
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();

}