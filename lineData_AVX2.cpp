#include <cmath>

#include "instrset.h"
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

void frameData::lineData::interpLinesDecompression_AVX2(void) {

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
void frameData::lineData::storeLinesDecompression_AVX2(T* out) {
	for (int component = 0; component < 3; component++) {
		auto compOutPtr = out + (height * width * component);
		store2out(outTopLine[component], compOutPtr + outTopOffset, width, parent.bitPerSubPixel);
		store2out(outBotLine[component], compOutPtr + outBotOffset, width, parent.bitPerSubPixel);
	}
}

template<typename T>
void frameData::lineData::gatherLinesCompression_AVX2(const T* in) {
	for (int i = 0; i < len; i++) {

		int x_access = i * (width / (float)len);

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (width * height * component);
			inTopLine[component][i] = compInPtr[x_access + y * width];
			inBotLine[component][i] = compInPtr[x_access + (height - 1 - y) * width];
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

void frameData::lineData::interpLinesCompression_AVX2(void) {

	if (parent.interp != nullptr) {
		for (int i = 0; i < len; i += Vec8f::size()) {
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
				int x_access = i + j - tapsOffset;
				x_access += (x_access < 0) * len;
				x_access -= (x_access >= len) * len;
				if ((x_access < 0) || (x_access >= len)) x_access = x_access % len;
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
void frameData::lineData::storeLinesCompression_AVX2(T* out) {
	for (int i = 0; i < len; ++i) {
		int x_access = xIndexes[i];
		int y_access = yIndexes[i];
		int component = 2;
		for (int component = 0; component < 3; component++) {
			T* compOutPtr = out + ((width / 2) * height * component);
			compOutPtr[x_access + y_access * width / 2] = std::max(std::min(outTopLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
			compOutPtr[x_access + (height - 1 - y_access) * width / 2] = std::max(std::min(outBotLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
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
		// 0, portanto temos o clamp de gra�a
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
