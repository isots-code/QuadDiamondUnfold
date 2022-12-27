#include <vector>
#include <array>
#include <cmath>

#include "extras.h"
#include "frameData.h"

#if INSTRSET >= 8 // AVX2
frameData::lineData::lineData(frameData& parent, int y)
	: op(parent.op), len(y * 4 + 2), width(parent.width), height(parent.height), y(y), taps(parent.taps), linePad(Vec8i::size()), paddedLen((len / linePad)* linePad + linePad),
	tapsOffset(-(taps / 2 - taps + 1)), outTopOffset(y * width), outBotOffset((height - 1 - y)* width), parent(parent) {
	xIndexes.resize(paddedLen);
	yIndexes.resize(paddedLen);
	lineIndexes.resize(height);
	coeffs.resize(taps);
	for (auto& subCoeffs : coeffs)
		subCoeffs.resize(height);
	op ? constructScatterLUT() : constructGatherLUT();
}
#else
frameData::lineData::lineData(frameData& parent, int y)
	: op(parent.op), len(y * 4 + 2), width(parent.width), height(parent.height), y(y), taps(parent.taps), linePad(1), paddedLen((len / linePad)* linePad + linePad),
	tapsOffset(-(taps / 2 - taps + 1)), outTopOffset(y* width), outBotOffset((height - 1 - y)* width), parent(parent) {
	xIndexes.resize(paddedLen);
	yIndexes.resize(paddedLen);
	lineIndexes.resize(height);
	coeffs.resize(taps);
	for (auto& subCoeffs : coeffs)
		subCoeffs.resize(height);
	op ? constructScatterLUT() : constructGatherLUT();
}
#endif

frameData::lineData::~lineData() {
	xIndexes.clear();
	yIndexes.clear();
	lineIndexes.clear();
	for (auto& subCoeffs : coeffs)
		subCoeffs.clear();
	coeffs.clear();
}

void frameData::lineData::decompressLine(const void* in, void* out) {
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
			gatherLinesDecompression(reinterpret_cast<const uint8_t*>(in));
			interpLinesDecompression();
			storeLinesDecompression(reinterpret_cast<uint8_t*>(out));
			break;
		case BITS_9:
		case BITS_10:
		case BITS_11:
		case BITS_12:
		case BITS_13:
		case BITS_14:
		case BITS_15:
		case BITS_16:
			gatherLinesDecompression(reinterpret_cast<const uint16_t*>(in));
			interpLinesDecompression();
			storeLinesDecompression(reinterpret_cast<uint16_t*>(out));
			break;
	}
}

void frameData::lineData::compressLine(const void* in, void* out) {
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
			gatherLinesCompression(reinterpret_cast<const uint8_t*>(in));
			interpLinesCompression();
			storeLinesCompression(reinterpret_cast<uint8_t*>(out));
			break;
		case BITS_9:
		case BITS_10:
		case BITS_11:
		case BITS_12:
		case BITS_13:
		case BITS_14:
		case BITS_15:
		case BITS_16:
			gatherLinesCompression(reinterpret_cast<const uint16_t*>(in));
			interpLinesCompression();
			storeLinesCompression(reinterpret_cast<uint16_t*>(out));
			break;
	}
}

void frameData::lineData::buildDecompressLineCoeffs(void) {

	const double distanceJ = len / (double)width;
	for (int x = 0; x < width / 2; x++) {
		auto x_ = distanceJ * x;
		x_ -= floor(x_);
		auto coeff = parent.coeffsFunc(x_);
		for (int i = 0; i < taps; i++)
			coeffs[i][x] = coeff[i];
	}
}

void frameData::lineData::buildCompressLineCoeffs(void) {

	const double distanceJ = len / (double)width;
	for (int x = 0; x < height; x++) {
		auto x_ = distanceJ * x;
		x_ -= floor(x_);
		auto coeff = parent.coeffsFunc(x_);
		for (int i = 0; i < taps; i++)
			coeffs[i][x] = coeff[i];
	}
}

#if INSTRSET >= 8 // AVX2
template<typename T>
void frameData::lineData::gatherLinesDecompression(const T* in) {
	for (unsigned long long i = 0; i < xIndexes.size(); i += Vec8us::size()) {

		Vec8i x = extend(Vec8us().load(&(xIndexes[i])));
		Vec8i y = extend(Vec8us().load(&(yIndexes[i])));

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (height * height * component);
			gather(compInPtr, x + y * height).store(&(inTopLine[component][i]));
			gather(compInPtr, x + (height - 1 - y) * height).store(&(inBotLine[component][i]));
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

void frameData::lineData::interpLinesDecompression(void) {

	const int Lj = len / 2;

	if (parent.interp != nullptr) {
		for (int i = 0; i < width; i += Vec8f::size()) {
			for (int component = 0; component < 3; component++) {
				parent.interp(*this, i, inTopLine[component], outTopLine[component]);
				parent.interp(*this, i, inBotLine[component], outBotLine[component]);
			}
		}
	} else {
		for (int i = 0; i < width / 2; i += Vec8f::size()) {
			Vec8f sumTR[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) },
				sumTL[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) },
				sumBR[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) },
				sumBL[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) };
			Vec8i indexes = extend(Vec8us().load(&(lineIndexes[i])));
			Vec8i baseIndex(lineIndexes[i]);

			for (int j = 0; j < taps; j++) {
				Vec8f coeff = Vec8f().load(&coeffs[j][i]);
				for (int component = 0; component < 3; component++) {
					sumTL[component] += lookup8(indexes - baseIndex, Vec8f().load(inTopLine[component] + lineIndexes[i] + j - tapsOffset)) * coeff;
					sumTR[component] += lookup8(indexes - baseIndex, Vec8f().load(inTopLine[component] + lineIndexes[i] + j - tapsOffset + Lj)) * coeff;
					sumBL[component] += lookup8(indexes - baseIndex, Vec8f().load(inBotLine[component] + lineIndexes[i] + j - tapsOffset)) * coeff;
					sumBR[component] += lookup8(indexes - baseIndex, Vec8f().load(inBotLine[component] + lineIndexes[i] + j - tapsOffset + Lj)) * coeff;
				}
			}

			for (int component = 0; component < 3; component++) {
				truncatei(sumTL[component]).store(&(outTopLine[component][i]));
				truncatei(sumTR[component]).store(&(outTopLine[component][i + height]));
				truncatei(sumBL[component]).store(&(outBotLine[component][i]));
				truncatei(sumBR[component]).store(&(outBotLine[component][i + height]));
			}
		}
	}
}

template<typename T>
void frameData::lineData::storeLinesDecompression(T* out) {
	for (int component = 0; component < 3; component++) {
		auto compOutPtr = out + (height * width * component);
		store2out(outTopLine[component], compOutPtr + outTopOffset, width);
		store2out(outBotLine[component], compOutPtr + outBotOffset, width);
	}
}

template void frameData::lineData::storeLinesDecompression(uint8_t* out); //????
#else
template<typename T>
void frameData::lineData::gatherLinesDecompression(const T* in) {
	for (unsigned long long i = 0; i < xIndexes.size(); i++) {

		int x_access = xIndexes[i];
		int y_access = yIndexes[i];

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (height * height * component);
			inTopLine[component][i] = compInPtr[x_access + y_access * height];
			inBotLine[component][i] = compInPtr[x_access + (height - 1 - y_access) * height];
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

void frameData::lineData::interpLinesDecompression(void) {

	const int Lj = len / 2;

	if (parent.interp != nullptr) {
		for (int i = 0; i < width; i++) {
			for (int component = 0; component < 3; component++) {
				parent.interp(*this, i, inTopLine[component], outTopLine[component]);
				parent.interp(*this, i, inBotLine[component], outBotLine[component]);
			}
		}
	} else {

		for (int i = 0; i < width / 2; i++) {

			float sumTR[3] = { 0.5f, 0.5f , 0.5f },
				sumTL[3] = { 0.5f, 0.5f , 0.5f },
				sumBR[3] = { 0.5f, 0.5f , 0.5f },
				sumBL[3] = { 0.5f, 0.5f , 0.5f };

			for (int j = 0; j < taps; j++) {
				float coeff = coeffs[j][i];
				for (int component = 0; component < 3; component++) {
					sumTL[component] += inTopLine[component][lineIndexes[i] + j - tapsOffset] * coeff;
					sumTR[component] += inTopLine[component][lineIndexes[i] + j - tapsOffset + Lj] * coeff;
					sumBL[component] += inBotLine[component][lineIndexes[i] + j - tapsOffset] * coeff;
					sumBR[component] += inBotLine[component][lineIndexes[i] + j - tapsOffset + Lj] * coeff;
				}
			}

			for (int component = 0; component < 3; component++) {
				outTopLine[component][i] = sumTL[component];
				outTopLine[component][i + height] = sumTR[component];
				outBotLine[component][i] = sumBL[component];
				outBotLine[component][i + height] = sumBR[component];
			}
		}
	}
}

template<typename T>
void frameData::lineData::storeLinesDecompression(T* out) {
	for (int i = 0; i < width; ++i) {
		for (int component = 0; component < 3; component++) {
			auto compOutPtr = out + (width * height * component);
			compOutPtr[i + outTopOffset] = std::max(std::min(outTopLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
			compOutPtr[i + outBotOffset] = std::max(std::min(outBotLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
		}
	}
}
#endif


template<typename T>
void frameData::lineData::gatherLinesCompression(const T* in) {
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

void frameData::lineData::interpLinesCompression(void) {

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
void frameData::lineData::storeLinesCompression(T* out) {
	for (int i = 0; i < len; ++i) {
		int x_access = xIndexes[i];
		int y_access = yIndexes[i];
		for (int component = 0; component < 3; component++) {
			auto compOutPtr = out + (width * height * component);
			compOutPtr[i + x_access + y_access * width] = std::max(std::min(outTopLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
			compOutPtr[i + x_access + (width - 1 - y_access) * height] = std::max(std::min(outBotLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
		}
	}
}

void frameData::lineData::constructGatherLUT(void) {

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

void frameData::lineData::constructScatterLUT(void) {

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

#if INSTRSET >= 8 // AVX2
template<>
void frameData::lineData::store2out(const int* in, uint8_t* out, int length) {
	int i = 0;
	for (; i < length - Vec32uc::size() - 1; i += Vec32uc::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		Vec8i c = Vec8i().load(in + i + 16);
		Vec8i d = Vec8i().load(in + i + 24);
		// usar packus aqui pq satura se os nrs forem maiores k 255 ou menores k
		// 0, portanto temos o clamp de graça
		Vec32uc(_mm256_packus_epi16(compress_saturated(a, c), compress_saturated(b, d))).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = std::max(std::min(in[i], 255), 0);

	return;
}

template<>
void frameData::lineData::store2out(const int* in, uint16_t* out, int length) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		compress_saturated(a, b).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = std::max(std::min(in[i], (1 << parent.bitPerSubPixel) - 1), 0);

	return;
}

#endif
