#include "frameData.h"

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, customInterp_t customInterp, int numThreads)
	: ThreadedExecutor((dim * dim * 3) * (op ? 2 : 1) * ((bits + 7) / 8), (dim * dim * 3) * (op ? 1 : 2) * ((bits + 7) / 8), numThreads)
	, bitPerSubPixel(bits), op(op), taps(taps), width(2 * dim), height(dim), interp(nullptr), customInterp(customInterp) {
	lines.reserve(height / 2);
	for (int i = 0; i < height / 2; i++)
		lines.emplace_back(*this, i);
}

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interp_t interp, int numThreads)
	: ThreadedExecutor((dim * dim * 3) * (op ? 2 : 1) * ((bits + 7) / 8), (dim * dim * 3) * (op ? 1 : 2)  * ((bits + 7) / 8), numThreads),
	bitPerSubPixel(bits), op(op), taps(taps), width(2 * dim), height(dim), interp(interp), customInterp(nullptr) {
	lines.reserve(height / 2);
	for (int i = 0; i < height / 2; i++)
		lines.emplace_back(*this, i);
}

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, customInterp_t customInterp) : frameData(op, dim, taps, bits, customInterp, std::thread::hardware_concurrency()) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits, customInterp_t customInterp, int numThreads) : frameData(op, dim, customInterp.taps, bits, customInterp, numThreads) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits, customInterp_t customInterp) : frameData(op, dim, customInterp.taps, bits, customInterp, std::thread::hardware_concurrency()) {}

frameData::frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interp_t interp) : frameData(op, dim, taps, bits, interp, std::thread::hardware_concurrency()) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits, interp_t interp, int numThreads) : frameData(op, dim, interp.taps, bits, interp, numThreads) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits, interp_t interp) : frameData(op, dim, interp.taps, bits, interp, std::thread::hardware_concurrency()) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits, int numThreads) : frameData(op, dim, interpolators[LANCZOS4].taps, bits, interpolators[LANCZOS4], numThreads) {}
frameData::frameData(bool op, int dim, bitPerSubPixel_t bits) : frameData(op, dim, interpolators[LANCZOS4].taps, bits, interpolators[LANCZOS4], std::thread::hardware_concurrency()) {}

frameData::~frameData() { this->stop(); }

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

void frameData::kernel(const int id) {
	for (int i = id; i < height / 2; i += this->numThreads) // topo e fundo por iteração
		op ? lines[i].compressLine(this->input, this->output) : lines[i].decompressLine(this->input, this->output);
};