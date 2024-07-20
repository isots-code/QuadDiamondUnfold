#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include <cmath>

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

#include "frameData.h"

template void frameData::lineData::gatherLinesDecompression_AVX2(const uint8_t* in);
template void frameData::lineData::storeLinesDecompression_AVX2(uint8_t* out);
template void frameData::lineData::gatherLinesDecompression_AVX2(const uint16_t* in);
template void frameData::lineData::storeLinesDecompression_AVX2(uint16_t* out);
template void frameData::lineData::gatherLinesCompression_AVX2(const uint8_t* in);
template void frameData::lineData::storeLinesCompression_AVX2(uint8_t* out);
template void frameData::lineData::gatherLinesCompression_AVX2(const uint16_t* in);
template void frameData::lineData::storeLinesCompression_AVX2(uint16_t* out);

template<typename T>
void store2out(const int* in, T* out, int length, frameData::bitPerSubPixel_t bits);

template <typename T>
Vec8f gather(const T* in, const Vec8i index) {
	if constexpr (std::same_as<T, float>)
		return _mm256_i32gather_ps(in, index, sizeof(*in));
	else {
		auto mask = Vec8i(UINT_MAX >> 8 * (4 - sizeof(T)));
		return to_float(mask & _mm256_i32gather_epi32(in, index, sizeof(*in)));
	}
}

template<typename T>
void frameData::lineData::gatherLinesDecompression_AVX2(const T* in) {
	const int halfWidth = width / 2;
	for (unsigned long long i = 0; i < xIndexes.size(); i += Vec8us::size()) {

		Vec8i x = extend(Vec8us().load(&(xIndexes[i])));
		Vec8i y = extend(Vec8us().load(&(yIndexes[i])));

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (halfWidth * height * component);
			gather(compInPtr, x + y * halfWidth).store(&(inTopLine[component][i]));
			gather(compInPtr, x + (height - 1 - y) * halfWidth).store(&(inBotLine[component][i]));
		}

	}
	//margins
	for (int i = 0; i < taps / 2; i++) {
		auto leftOffset = (i - tapsOffset) % lenghtJ;
		leftOffset += leftOffset < 0 ? lenghtJ : 0;
		auto rightOffset = (lenghtJ + i) % lenghtJ;
		rightOffset += rightOffset < 0 ? lenghtJ : 0;
		for (int component = 0; component < 3; component++) {
			inTopLine[component][i - tapsOffset] = inTopLine[component][leftOffset];
			inTopLine[component][i + lenghtJ] = inTopLine[component][rightOffset];
			inBotLine[component][i - tapsOffset] = inBotLine[component][leftOffset];
			inBotLine[component][i + lenghtJ] = inBotLine[component][rightOffset];
		}
	}
}

void frameData::lineData::interpLinesDecompression_AVX2(void) {

	const int Lj = lenghtJ / 2;

	if (parent.customInterp.func != nullptr) {
		for (int i = 0; i < width; i += Vec8f::size()) {
			for (int component = 0; component < 3; component++) {
				parent.customInterp.func(false, width, lenghtJ, i, inTopLine[component], outTopLine[component]);
				parent.customInterp.func(false, width, lenghtJ, i, inBotLine[component], outBotLine[component]);
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
void frameData::lineData::storeLinesDecompression_AVX2(T* out) {
	for (int component = 0; component < 3; component++) {
		auto compOutPtr = out + (height * width * component);
		store2out(outTopLine[component], compOutPtr + outTopOffset, width, parent.bitPerSubPixel);
		store2out(outBotLine[component], compOutPtr + outBotOffset, width, parent.bitPerSubPixel);
	}
}

template<typename T>
void frameData::lineData::gatherLinesCompression_AVX2(const T* in) {
	for (int component = 0; component < 3; component++) {
		auto compInPtr = in + (width * height * component);
		for (int i = 0; i < width; i += Vec8i::size()) {

			Vec8i x_access = Vec8i(i) + Vec8i(0, 1, 2, 3, 4, 5, 6, 7);
			x_access = (x_access < width) & x_access;

			gather(compInPtr, x_access + y * width).store(&(inTopLine[component][i]));
			gather(compInPtr, x_access + (height - 1 - y) * width).store(&(inBotLine[component][i]));
		}

	}

	//margins
	for (int i = 0; i < taps / 2; i++) {
		auto leftOffset = (i - tapsOffset) % width;
		leftOffset += leftOffset < 0 ? width : 0;
		auto rightOffset = (width + i) % width;
		rightOffset += rightOffset < 0 ? width : 0;
		for (int component = 0; component < 3; component++) {
			inTopLine[component][i - tapsOffset] = inTopLine[component][leftOffset];
			inTopLine[component][i + width] = inTopLine[component][rightOffset];
			inBotLine[component][i - tapsOffset] = inBotLine[component][leftOffset];
			inBotLine[component][i + width] = inBotLine[component][rightOffset];
		}
	}
}

void frameData::lineData::interpLinesCompression_AVX2(void) {

	const Vec8i clamp_min(0);
	const Vec8i clamp_max((1 << parent.bitPerSubPixel) - 1);
	if (parent.customInterp.func != nullptr) {
		for (int i = 0; i < lenghtJ; i += Vec8f::size()) {
			for (int component = 0; component < 3; component++) {
				parent.customInterp.func(true, width, lenghtJ, i, inTopLine[component], outTopLine[component]);
				parent.customInterp.func(true, width, lenghtJ, i, inBotLine[component], outBotLine[component]);
				Vec8i tempTop = Vec8i().load(&outTopLine[component][i]);
				Vec8i tempBot = Vec8i().load(&outBotLine[component][i]);
				max(min(tempTop, clamp_max), clamp_min).store(&(outTopLine[component][i]));
				max(min(tempBot, clamp_max), clamp_min).store(&(outBotLine[component][i]));
			}
		}
	} else {
		const double distanceJ = (double)width / lenghtJ;
		for (int i = 0; i < lenghtJ; i += Vec8f::size()) {

			Vec8f sumTop[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) },
				sumBot[3] = { Vec8f(0.5f), Vec8f(0.5f) , Vec8f(0.5f) };

			Vec8i start_x = truncatei(to_float(Vec8d(i + 0, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7) * distanceJ));
			start_x = (start_x < width + taps) & start_x;
			for (int tap = 0; tap < taps; tap++, start_x++) {
				Vec8i x_access = start_x - tapsOffset;
				Vec8f coeff = Vec8f().load(&coeffs[tap][i]);
				for (int component = 0; component < 3; component++) {
					sumTop[component] += gather(inTopLine[component], x_access) * coeff;
					sumBot[component] += gather(inBotLine[component], x_access) * coeff;
				}
			}

			for (int component = 0; component < 3; component++) {
				max(min(truncatei(sumTop[component]), clamp_max), clamp_min).store(&(outTopLine[component][i]));
				max(min(truncatei(sumBot[component]), clamp_max), clamp_min).store(&(outBotLine[component][i]));
			}
		}
	}
}

template<typename T>
void frameData::lineData::storeLinesCompression_AVX2(T* out) {
	const int halfWidth = width / 2;
	for (int i = 0; i < lenghtJ; ++i) {
		int x_access = xIndexes[i];
		int y_access = yIndexes[i];
		for (int component = 0; component < 3; component++) {
			T* compOutPtr = out + (halfWidth * height * component);
			compOutPtr[x_access + y_access * halfWidth] = outTopLine[component][i];
			compOutPtr[x_access + (height - 1 - y_access) * halfWidth] = outBotLine[component][i];
		}
	}
}

template<>
void store2out(const int* in, uint8_t* out, int length, frameData::bitPerSubPixel_t bits) {
	(void)bits;
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
void store2out(const int* in, uint16_t* out, int length, frameData::bitPerSubPixel_t bits) {
	int i = 0;
	for (; i < length - Vec16us::size() - 1; i += Vec16us::size()) {
		Vec8i a = Vec8i().load(in + i);
		Vec8i b = Vec8i().load(in + i + 8);
		compress_saturated(a, b).store_nt(out + i);
	}

	// remainder loop
#pragma clang loop vectorize(disable)
	for (; i < length; ++i)
		out[i] = std::max(std::min(in[i], (1 << bits) - 1), 0);

	return;
}

