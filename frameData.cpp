#include "frameData.h"

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads)
	: ThreadedExecutor(dim, numThreads), bitPerSubPixel(bits), op(op), taps(taps),
		width(2 * dim), height(dim), interp(interp) {
	lines.reserve(height / 2);
	for (int i = 0; i < height / 2; i++)
		lines.emplace_back(*this, i);
}

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp) : frameData(op, dim, taps, bits, interp, std::thread::hardware_concurrency()) {}
frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, int numThreads) : frameData(op, dim, taps, bits, nullptr, numThreads) {}
frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits) : frameData(op, dim, taps, bits, nullptr, std::thread::hardware_concurrency()) {}

frameData::~frameData() {
	this->stop();
	lines.clear();
}

#if INSTRSET >= 8 // AVX2
template<>
void frameData::expandUV(uint8_t* data, int width, int height) {

	struct wrapper {
		uint8_t* data;
		const size_t width;
		const size_t height;
		wrapper(uint8_t* data, const size_t width, const size_t height) : data(data), width(width), height(height) {};
		uint8_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * width + (height * width * comp)]; };
	};

	auto temp = new uint8_t[(width / 2) * (height / 2) * 2];
	std::memcpy(temp, data, (width / 2)* (height / 2) * 2);
	wrapper input(temp, width / 2, height / 2);
	wrapper output(data, width, height);

	
	const Vec32uc LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15);

	for (int y = 0; y < height / 2; y++) {
		int x = 0;
		for (; x < (width / 2) - (Vec16uc::size() - 1); x += Vec16uc::size()) {
			for (int comp = 0; comp < 2; comp++) {
				Vec32uc in(Vec16uc().load(&input.at(x, y, comp)), Vec16uc());
				Vec32uc out(lookup32(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			}
		}

		for (; x < width / 2; ++x) {
			for (int comp = 0; comp < 2; comp++)
				output.at(2 * x, 2 * y, comp) =
				output.at(2 * x + 1, 2 * y, comp) =
				output.at(2 * x, 2 * y + 1, comp) =
				output.at(2 * x + 1, 2 * y + 1, comp) =
				input.at(x, y, comp);
		}
	}

	delete[] temp;

}

template<>
void frameData::expandUV(uint16_t* data, int width, int height) {

	struct wrapper {
		uint16_t* data;
		const size_t width;
		const size_t height;
		wrapper(uint16_t* data, const size_t width, const size_t height) : data(data), width(width), height(height) {};
		uint16_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * width + (height * width * comp)]; };
	};

	auto temp = new uint16_t[(width / 2) * (height / 2) * 2];
	std::memcpy(temp, data, (width / 2)* (height / 2) * 2);
	wrapper input(temp, width / 2, height / 2);
	wrapper output(data, width, height);
	
	const Vec16us LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7);

	for (int y = 0; y < height / 2; y++) {
		int x = 0;
		for (; x < (width / 2) - (Vec8us::size() - 1); x += Vec8us::size()) {
			for (int comp = 0; comp < 2; comp++) {
				Vec16us in(Vec8us().load(&input.at(x, y, comp)), Vec8us());
				Vec16us out(lookup16(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			}
		}

		for (; x < width / 2; ++x) {
			for (int comp = 0; comp < 2; comp++)
				output.at(2 * x, 2 * y, comp) =
				output.at(2 * x + 1, 2 * y, comp) =
				output.at(2 * x, 2 * y + 1, comp) =
				output.at(2 * x + 1, 2 * y + 1, comp) =
				input.at(x, y, comp);
		}
	}

	delete[] temp;

}
#else
template<typename T>
void frameData::expandUV(T* data, int width, int height) {

	struct wrapper {
		T* data;
		const size_t width;
		const size_t height;
		wrapper(T* data, const size_t width, const size_t height) : data(data), width(width), height(height) {};
		T& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * width + (height * width * comp)]; };
	};

	auto temp = new T[(width / 2) * (height / 2) * 2];
	std::memcpy(temp, data, (width / 2) * (height / 2) * 2);
	wrapper input(temp, width / 2, height / 2);
	wrapper output(data, width, height);

	for (int y = 0; y < height / 2; y++) {
		for (int x = 0; x < width / 2; ++x) {
			for (int comp = 0; comp < 2; comp++)
				output.at(2 * x, 2 * y, comp) =
				output.at(2 * x + 1, 2 * y, comp) =
				output.at(2 * x, 2 * y + 1, comp) =
				output.at(2 * x + 1, 2 * y + 1, comp) =
				input.at(x, y, comp);
		}
	}

	delete[] temp;

}

template void frameData::expandUV(uint8_t* data, int width, int height);
template void frameData::expandUV(uint16_t* data, int width, int height);

#endif

void frameData::buildFrameCoeffs(void) {
	for (auto& line : lines)
		op ? line.buildCompressLineCoeffs() : line.buildDecompressLineCoeffs();
}

void frameData::kernel(const int id) {
	for (int i = id; i < height / 2; i += this->numThreads) // topo e fundo por iteração
		op ? lines[i].compressLine(this->input, this->output) : lines[i].decompressLine(this->input, this->output);
};

std::vector<float> frameData::coeffsFunc(double x) {
	(void)x;
	return std::vector<float>(taps, 0.0f);
}
