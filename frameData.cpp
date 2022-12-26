#include "frameData.h"

frameData::frameData(int dim, int taps, bitPerSubPixel_t bits, int numThreads) : ThreadedExecutor(dim, numThreads), bitPerSubPixel(bits), dim(dim), taps(taps) {
	lines.reserve(dim / 2);
	for (int i = 0; i < dim / 2; i++)
		lines.emplace_back(*this, i, dim);
}

frameData::frameData(int dim, int taps, bitPerSubPixel_t bits) : frameData(dim, taps, bits, std::thread::hardware_concurrency()) {}

frameData::~frameData() {
	this->stop();
	lines.clear();
}

template<>
void frameData::expandUV(uint8_t* data, int dim) {

	struct wrapper {
		uint8_t* data;
		size_t dim;
		wrapper(uint8_t* data, const size_t dim) : data(data), dim(dim) {};
		uint8_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * dim + (dim * dim * comp)]; };
	};

	auto temp = new uint8_t[dim * dim / 2];
	std::memcpy(temp, data, dim * dim / 2);
	wrapper input(temp, dim / 2);
	wrapper output(data, dim);

	const Vec32uc LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15);

	for (int y = 0; y < dim / 2; y++) {
		int x = 0;
		for (; x < (dim / 2) - (Vec16uc::size() - 1); x += Vec16uc::size()) {
			for (int comp = 0; comp < 2; comp++) {
				Vec32uc in(Vec16uc().load(&input.at(x, y, comp)), Vec16uc());
				Vec32uc out(lookup32(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			}
		}

		for (; x < dim / 2; ++x) {
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
void frameData::expandUV(uint16_t* data, int dim) {

	struct wrapper {
		uint16_t* data;
		size_t dim;
		wrapper(uint16_t* data, const size_t dim) : data(data), dim(dim) {};
		uint16_t& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * dim + (dim * dim * comp)]; };
	};

	auto temp = new uint16_t[dim * dim / 2];
	std::memcpy(temp, data, dim * dim / 2);
	wrapper input(temp, dim / 2);
	wrapper output(data, dim);

	const Vec16us LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7);

	for (int y = 0; y < dim / 2; y++) {
		int x = 0;
		for (; x < (dim / 2) - (Vec8us::size() - 1); x += Vec8us::size()) {
			for (int comp = 0; comp < 2; comp++) {
				Vec16us in(Vec8us().load(&input.at(x, y, comp)), Vec8us());
				Vec16us out(lookup16(LUT, in));
				out.store(&output.at(2 * x, 2 * y, comp));
				out.store(&output.at(2 * x, 2 * y + 1, comp));
			}
		}

		for (; x < dim / 2; ++x) {
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

void frameData::buildFrameCoeffs(void) {
	for (auto& line : lines)
		line.buildLineCoeffs();
}

void frameData::kernel(const int id) {
	for (int i = id; i < dim / 2; i += this->numThreads) // topo e fundo por iteração
		lines[i].processLine(this->input, this->output);
};
