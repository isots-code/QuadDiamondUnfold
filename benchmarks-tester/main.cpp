#include "AlignedVector.h"
#include "frameData.h"

#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif


extern void bench_og(benchmark::State& s);
extern char (*inter[8])(char*, double, int);

using namespace std::chrono;

template <typename T, class ...Args>
static void bench(benchmark::State& s, bool op, Args&&... args) {

	// Threads
	auto threads = s.range(0);

	// Number of elements
	auto dim = 8 << s.range(1);

	// Taps
	auto taps = s.range(2);

	std::vector<T> in(dim * dim * 3);

	// Variable for our results
	AlignedVector<T> out(dim * dim * 2 * 3);

	frameData* benchData;
	if (taps == 1)
		benchData = new frameData(op, dim, std::forward<Args>(args)..., threads);
	else
		benchData = new frameData(op, dim, taps, std::forward<Args>(args)..., threads);

#ifdef _WIN32
	//SetProcessAffinityMask(GetCurrentProcess(), 0x30);
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

	benchData->setIOBuffers(in.data(), in.data(), out.data(), out.data());
	// Main timing loop
	for (auto _ : s) {
		benchData->writeInput();
		benchData->expandUV(in.data() + dim * dim, dim, dim);
		benchData->readOutput();
	}

#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif

	delete benchData;

	//s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2 * sizeof(T))); //read or write BW
	//s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2 * sizeof(T) * 1.5)); //both
	s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2 * sizeof(T))); // pixels/s output
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

//template <class benchStruct>
//static void bench_file(benchmark::State& s, char** input) {
//
//	using T = typename benchStruct::DataType;
//
//	// Taps
//	auto taps = s.range(0);
//
//	std::string path = input[0];
//	std::string file = input[1];
//
//	//printf("%s\n", (path + file).c_str());
//
//	int dim;
//	sscanf(strchr(file.c_str(), 'x'), "x%d#", &dim);
//
//	//dont need this to start making values
//	FILE* file_ptr = fopen((path + file).c_str(), "rb");
//	if (!file_ptr) {
//		s.SkipWithError("Error reading input file");
//		return;
//	}
//
//	std::vector<T> image(dim * dim * 3 * 2);
//	fread(image.data(), dim * dim * 3, 1, file_ptr);
//	fclose(file_ptr);
//
//	benchStruct::expandUV(image.data() + dim * dim, dim);
//
//	// Variable for our results
//	AlignedVector<T> out(dim * dim * 2 * 3); 
//	
//	{
//		//benchStruct benchData(dim, taps, 1);
//		//benchStruct benchData(dim, taps);
//		benchStruct benchData(dim, taps, &centrip_catmull_rom);
//
//		benchData.setIOBuffers(image.data(), image.data(), out.data(), out.data());
//
//		std::vector<T> temp(dim * dim * 3 * 2);
//
//#ifdef _WIN32
//		//SetProcessAffinityMask(GetCurrentProcess(), 0x30);
//		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
//#endif
//
//		// Main timing loop
//		for (auto _ : s) {
//			benchData.writeInput();
//			benchData.readOutput();
//		}
//
//#ifdef _WIN32
//		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
//#endif
//
//	}
//
//	s.SetBytesProcessed(uint64_t(s.iterations() * dim * dim * 2));
//	s.SetItemsProcessed(s.iterations());
//	s.SetLabel([](int size) {
//		size_t dim = size * size * 2;
//		if (dim < (1 << 10))
//			return std::to_string(dim) + " pixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
//		if (dim < (1 << 20))
//			return std::to_string(dim >> 10) + " kpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
//		if (dim < (1 << 30))
//			return std::to_string(dim >> 20) + " Mpixels (" + std::to_string(size * 2) + " * " + std::to_string(size) + ")";
//		return std::string("Empty");
//	} (dim));
//
//	char image_path[10240];
//	(void)sprintf(image_path, "%s%s_decomp_simd_threaded.yuv", path.c_str(), strtok(const_cast<char*>(file.c_str()), "#"));
//	file_ptr = fopen(image_path, "wb");
//	fwrite(out.data(), 1, dim * dim * 2 * 3, file_ptr);
//	fclose(file_ptr);
//
//	return;
//}

#define TEST(name, func, args, ...) benchmark::RegisterBenchmark(name, [=](auto& st) { func(st, __VA_ARGS__); })->args->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::TimeUnit::kMillisecond)

int main(int argc, char** argv) {

	/*//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameDataCustom<bits8_t>>), &(argv[1]))->FILEARGS;
	//benchmark::RegisterBenchmark(EXPAND_TEMPLATE_BENCH(bench_file<frameData<bits8_t>>), &(argv[1]))->FILEARGS;
	
	//TEST("bench_8bit_nearest", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, interpolators[interpolator::NEAREST]);
	//TEST("bench_8bit_linear", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, interpolators[interpolator::LINEAR]);
	//TEST("bench_8bit_cubic", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, interpolators[interpolator::CUBIC]);
	//TEST("bench_8bit_catmull", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, interpolators[interpolator::CATMULL_ROM]);
	//TEST("bench_8bit_centrip", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, customInterpolators[interpolator::CENTRIPETAL_CATMULL_ROM]);
	//TEST("bench_8bit_lancz4", bench<uint8_t>, ARGS({ 1 }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4]);
	//TEST("bench_8bit_lanc_n", bench<uint8_t>, ARGS(TAPS), 0, frameData::BITS_8, interpolators[interpolator::LANCZOSN]);

	//TEST("bench_10bit_nearest", bench<uint16_t>, ARGS({ 1 }), 0, frameData::BITS_10, interpolators[interpolator::NEAREST]);
	//TEST("bench_12bit_nearest", bench<uint16_t>, ARGS({ 1 }), 0, frameData::BITS_12, interpolators[interpolator::NEAREST]);
	//TEST("bench_14bit_nearest", bench<uint16_t>, ARGS({ 1 }), 0, frameData::BITS_14, interpolators[interpolator::NEAREST]);
	//TEST("bench_16bit_nearest", bench<uint16_t>, ARGS({ 1 }), 0, frameData::BITS_16, interpolators[interpolator::NEAREST]);

	for (int i = 0; i < (sizeof(inter)/sizeof(*inter)); i++)
		benchmark::RegisterBenchmark("bench_og_interps", bench_og)->Args({ 6, i })->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::TimeUnit::kMillisecond)->MinTime(1);

	for (int i = 6; i <= 10; i++) {
		benchmark::RegisterBenchmark("bench_og", bench_og)->Args({ i, 5 })->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::TimeUnit::kMillisecond)->MinTime(1);
		TEST("bench_scalar_single", bench<uint8_t>, ArgsProduct({ { 1 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], false);
		TEST("bench_scalar_multi", bench<uint8_t>, ArgsProduct({ { 16 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], false);
		TEST("bench_avx_single", bench<uint8_t>, ArgsProduct({ { 1 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], true);
		TEST("bench_avx_multi", bench<uint8_t>, ArgsProduct({ { 16 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], true);

	//for (int i = 6; i <= 10; i++) {
	//	TEST("bench_og", bench_og, Args({ i, 5 }));
	//	TEST("bench_scalar_single", bench<uint8_t>, ArgsProduct({ { 1 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], false);
	//	TEST("bench_scalar_multi", bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], false);
	//	TEST("bench_avx_single", bench<uint8_t>, ArgsProduct({ { 1 }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], true);
	//	TEST("bench_avx_multi", bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { i }, { 1 } }), 0, frameData::BITS_8, interpolators[interpolator::LANCZOS4], true);
	//}*/
	
	for (const auto size : benchmark::CreateDenseRange(6, 10, 1)) {

		//const auto interp = interpolators[0];
		//TEST("expand", bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { 1 } }), 0, frameData::BITS_8, interp, false);

		for (unsigned int i = 0; i < (sizeof(inter) / sizeof(*inter)); i++)
			TEST("bench_og", bench_og, ArgsProduct({ { size },  { i } }));

		for (const auto& interp : std::vector<interp_t>(std::begin(interpolators), std::end(interpolators) - 1)) {
			TEST("bench_scalar_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { 1 } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_scalar_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { 1 } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_avx_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { 1 } }), 0, frameData::BITS_8, interp, true);
			TEST("bench_avx_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { 1 } }), 0, frameData::BITS_8, interp, true);
		}

		for (const auto taps : benchmark::CreateRange(2, 16, 2)) {
			const auto& interp = interpolators[interpolator::LANCZOSN];
			TEST("bench_scalar_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { taps } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_scalar_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { taps } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_avx_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { taps } }), 0, frameData::BITS_8, interp, true);
			TEST("bench_avx_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { taps } }), 0, frameData::BITS_8, interp, true);
		}

		for (const auto& interp : customInterpolators) {
			TEST("bench_scalar_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { 1 } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_scalar_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { 1 } }), 0, frameData::BITS_8, interp, false);
			TEST("bench_avx_single_" + interp.name, bench<uint8_t>, ArgsProduct({ { 1 }, { size }, { 1 } }), 0, frameData::BITS_8, interp, true);
			TEST("bench_avx_multi_" + interp.name, bench<uint8_t>, ArgsProduct({ { std::thread::hardware_concurrency() }, { size }, { 1 } }), 0, frameData::BITS_8, interp, true);
		}
	}


	//these entries are from BENCHMARK_MAIN
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
	benchmark::Shutdown();

}
