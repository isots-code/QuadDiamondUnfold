#include "Compressor.h"
#include "extras.h"

Compressor::Compressor(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThreads) : frameData(dim, taps, bits, numThreads), interp(interp){
	linesCompressor.reserve(dim / 2);
	for (int i = 0; i < dim / 2; i++)
		linesCompressor.emplace_back(*this, i, dim);
}

Compressor::Compressor(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp) : Compressor(dim, taps, bits, interp, std::thread::hardware_concurrency()) {}

Compressor::Compressor(int dim, int taps, bitPerSubPixel_t bits, int numThreads) : Compressor(dim, taps, bits, nullptr, numThreads) {}

Compressor::Compressor(int dim, int taps, bitPerSubPixel_t bits) : Compressor(dim, taps, bits, nullptr, std::thread::hardware_concurrency()) {}

Compressor::~Compressor() {
	stop();
	linesCompressor.clear();
}

std::vector<float> Compressor::coeffsFunc(double x) {
	(void)x;
	return std::vector<float>(taps, 1.0f);
}

void Compressor::kernel(const int id) {
	for (int i = id; i < dim / 2; i += numThreads) // topo e fundo por iteração
		linesCompressor[i].processLine(this->input, this->output);
};

Compressor::lineDataCompressor::lineDataCompressor(Compressor& parent, int y, int height) : frameData::lineData(parent, y, height), parent(parent) {}

void Compressor::lineDataCompressor::processLine(const void* in, void* out) {
	float inTopArray[3][paddedLen + taps];
	float inBotArray[3][paddedLen + taps];
	int outTopArray[3][width];
	int outBotArray[3][width];
	inTopLine = { inTopArray[0] + tapsOffset, inTopArray[1] + tapsOffset, inTopArray[2] + tapsOffset };
	inBotLine = { inBotArray[0] + tapsOffset, inBotArray[1] + tapsOffset, inBotArray[2] + tapsOffset };
	outTopLine = { outTopArray[0] + tapsOffset, outTopArray[1] + tapsOffset, outTopArray[2] + tapsOffset };
	outBotLine = { outBotArray[0] + tapsOffset, outBotArray[1] + tapsOffset, outBotArray[2] + tapsOffset };
	switch (this->parent.bitPerSubPixel) {
		case BITS_8:
			gatherLines(reinterpret_cast<const uint8_t*>(in));
			interpLines();
			storeLines(reinterpret_cast<uint8_t*>(out));
			break;
		case BITS_9:
		case BITS_10:
		case BITS_11:
		case BITS_12:
		case BITS_13:
		case BITS_14:
		case BITS_15:
		case BITS_16:
			gatherLines(reinterpret_cast<const uint16_t*>(in));
			interpLines();
			storeLines(reinterpret_cast<uint16_t*>(out));
			break;
	}
}

template<typename T>
void Compressor::lineDataCompressor::gatherLines(const T* in) {
	for (int i = 0; i < len; i++) {

		int x_access = i * (len / width);

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (height * height * component);
			inTopLine[component][i] = compInPtr[x_access + y * height];
			inBotLine[component][i] = compInPtr[x_access + (height - 1 - y) * height];
		}

	}
	//margins
	for (int i = 0; i < taps / 2; i++) {
		auto leftOffset = (i - tapsOffset) % len;
		leftOffset += leftOffset < 0 ? len : 0;
		auto rightOffset = (len + i) % len;
		rightOffset += rightOffset < 0 ? len : 0;
		for (int component = 0; component < 3; component++) {
			inTopLine[component][i - tapsOffset] = inTopLine[component][leftOffset];
			inTopLine[component][i + len] = inTopLine[component][rightOffset];
			inBotLine[component][i - tapsOffset] = inBotLine[component][leftOffset];
			inBotLine[component][i + len] = inBotLine[component][rightOffset];
		}
	}
}

void Compressor::lineDataCompressor::interpLines(void) {

	if (parent.interp != nullptr) {

#if INSTRSET >= 8 // AVX2
		int increment = Vec8f::size();
#else
		int increment = 1;
#endif
		for (int i = 0; i < len; i += increment) {
			for (int component = 0; component < 3; component++) {
				parent.interp(*this, i, inTopLine[component], outTopLine[component]);
				parent.interp(*this, i, inBotLine[component], outBotLine[component]);
			}
		}
	} else {
		for (int i = 0; i < len; i++) {

			float sumTop[3] = { 0.5f, 0.5f, 0.5f },
				sumBot[3] = { 0.5f, 0.5f, 0.5f };

			for (int j = 0; j < taps; j++) {
				float coeff = coeffs[j][i];
				int x_access = j - tapsOffset;
				x_access += (x_access < 0) * width;
				x_access -= (x_access >= width) * width;
				if ((x_access < 0) || (x_access >= width)) x_access = x_access % width;
				for (int component = 0; component < 3; component++) {
					sumTop[component] += inTopLine[component][x_access] * coeff;
					sumBot[component] += inBotLine[component][x_access] * coeff;
				}
			}

			for (int component = 0; component < 3; component++) {
				outTopLine[component][i] = sumTop[component];
				outBotLine[component][i] = sumBot[component];
			}
		}
	}
}

template<typename T>
void Compressor::lineDataCompressor::storeLines(T* out) {
	for (int i = 0; i < width; ++i) {
		int x_access = xIndexes[i];
		int y_access = yIndexes[i];
		for (int component = 0; component < 3; component++) {
			auto compOutPtr = out + (width * height * component);
			compOutPtr[i + x_access + y_access * width] = std::max(std::min(outTopLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
			compOutPtr[i + x_access + (width - 1 - y_access) * height] = std::max(std::min(outBotLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
		}
	}
}

void Compressor::lineDataCompressor::constructGatherLUT(void) {

	const int Lj = len / 2;
	const int halfheight = height / 2;
	const int x_inner_offset = halfheight - 2 * y - 1;
	const int x_left_offset = halfheight - y - 1;
	const int x_right_offset = halfheight + y;

	for (int x = 0; x < len; x++) {
		int temp = abs(Lj - 0.5 - x);
		if (y > temp) {
			yIndexes[x] = y;
			xIndexes[x] = x + x_inner_offset;
		} else {
			yIndexes[x] = Lj - temp - 1;
			xIndexes[x] = (x < Lj) ? x_left_offset : x_right_offset;
		}
	}

	const double Dj = Lj / (double)height;
	for (int x = 0; x < height; x++)
		lineIndexes[x] = std::floor(Dj * x);
}
