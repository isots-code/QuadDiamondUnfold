#define _CRT_SECURE_NO_WARNINGS

#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdint>
#include <vector>

#include <Windows.h>

#include "frameData.h"
#include "frameDataCustom.h"
#include "AlignedVector.h"
#include "ffmpeg.h"

template <class benchStruct>
static void bench(benchmark::State& s) {

	using T = typename benchStruct::DataType;

	// Taps
	auto taps = s.range(0);

	// Number of elements
	auto dim = 8 << s.range(1);

	// Create a vector of random numbers
	std::vector<T> in(dim * dim * 3);
	std::generate(begin(in), end(in), [] { return T{ .i = static_cast<decltype(T::i)>(rand() % 255) }; });

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
			benchData.writeInput();
			benchData.readOutput();
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

int main(int argc, char** argv) {

	using bitDepth = bits8_t;

	try {
		auto file = ffmpegInput<bitDepth>(std::string(argv[1]));

		int dim = file.getDim();
		auto dataLookup = frameDataCustom<bitDepth>(dim, 4, &centripetalCatMullRomInterpolation<bitDepth>);

		file.connectFrameData(dataLookup);

		AlignedVector<bitDepth> out0(dim * dim * 2 * 3);
		AlignedVector<bitDepth> out1(dim * dim * 2 * 3);

		dataLookup.setOBuffers(out0.data(), out1.data());

		file.startDecode();

		while (file.running);

	} catch (std::exception& e) {
		std::cerr << e.what();
		return -1;
	}

	return 0;
}