#include <cmath>

#include "frameData.h"

frameData::lineData::lineData(frameData& parent, int y)
	: op(parent.op), lenghtJ(y * 4 + 2), width(parent.width), height(parent.height), 
	y(y), taps(parent.taps), linePad(parent.simd ? 8 : 1), paddedLen((lenghtJ / linePad)* linePad + linePad),
	tapsOffset(-(taps / 2 - taps + 1)), outTopOffset(y * width), outBotOffset((height - 1 - y)* width), parent(parent) {
	xIndexes.resize(paddedLen);
	yIndexes.resize(paddedLen);
	lineIndexes.resize(op ? 0 : width / 2);
	coeffs.resize(taps);
	for (auto& subCoeffs : coeffs)
		subCoeffs.resize(op ? lenghtJ : width / 2);
	constructLUT();
}

frameData::lineData::~lineData() {}

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
			if (parent.simd) {
				gatherLinesDecompression_AVX2(reinterpret_cast<const uint8_t*>(in));
				interpLinesDecompression_AVX2();
				storeLinesDecompression_AVX2(reinterpret_cast<uint8_t*>(out));
			} else {
				gatherLinesDecompression(reinterpret_cast<const uint8_t*>(in));
				interpLinesDecompression();
				storeLinesDecompression(reinterpret_cast<uint8_t*>(out));
			}
			break;
		case BITS_9:
		case BITS_10:
		case BITS_11:
		case BITS_12:
		case BITS_13:
		case BITS_14:
		case BITS_15:
		case BITS_16:
			if (parent.simd) {
				gatherLinesDecompression_AVX2(reinterpret_cast<const uint16_t*>(in));
				interpLinesDecompression_AVX2();
				storeLinesDecompression_AVX2(reinterpret_cast<uint16_t*>(out));
			} else {
				gatherLinesDecompression(reinterpret_cast<const uint16_t*>(in));
				interpLinesDecompression();
				storeLinesDecompression(reinterpret_cast<uint16_t*>(out));
			}
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
			if (parent.simd) {
				gatherLinesCompression_AVX2(reinterpret_cast<const uint8_t*>(in));
				interpLinesCompression_AVX2();
				storeLinesCompression_AVX2(reinterpret_cast<uint8_t*>(out));
			} else {
				gatherLinesCompression(reinterpret_cast<const uint8_t*>(in));
				interpLinesCompression();
				storeLinesCompression(reinterpret_cast<uint8_t*>(out));
			}
			break;
		case BITS_9:
		case BITS_10:
		case BITS_11:
		case BITS_12:
		case BITS_13:
		case BITS_14:
		case BITS_15:
		case BITS_16:
			if (parent.simd) {
				gatherLinesCompression_AVX2(reinterpret_cast<const uint16_t*>(in));
				interpLinesCompression_AVX2();
				storeLinesCompression_AVX2(reinterpret_cast<uint16_t*>(out));
			} else {
				gatherLinesCompression(reinterpret_cast<const uint16_t*>(in));
				interpLinesCompression();
				storeLinesCompression(reinterpret_cast<uint16_t*>(out));
			}
			break;
	}
}

void frameData::lineData::buildDecompressLineCoeffs(void) {
	const double distanceJ = lenghtJ / (double)width;
	for (int x = 0; x < width / 2; x++) {
		auto x_ = distanceJ * x;
		x_ -= floor(x_);
		auto coeff = parent.interp.func(x_, taps);
		for (int i = 0; i < taps; i++)
			coeffs[i][x] = coeff[i];
	}
}

void frameData::lineData::buildCompressLineCoeffs(void) {
	const double distanceJ = lenghtJ / (double)width;
	for (int x = 0; x < lenghtJ; x++) {
		auto x_ = distanceJ * x;
		x_ -= floor(x_);
		auto coeff = parent.interp.func(x_, taps);
		for (int i = 0; i < taps; i++)
			coeffs[i][x] = coeff[i];
	}
}

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

void frameData::lineData::interpLinesDecompression(void) {

	const int Lj = lenghtJ / 2;

	if (parent.customInterp.func != nullptr) {
		for (int i = 0; i < width; i++) {
			for (int component = 0; component < 3; component++) {
				parent.customInterp.func(false, width, lenghtJ, i, inTopLine[component], outTopLine[component]);
				parent.customInterp.func(false, width, lenghtJ, i, inBotLine[component], outBotLine[component]);
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

template<typename T>
void frameData::lineData::gatherLinesCompression(const T* in) {
	for (int i = 0; i < lenghtJ; i++) {

		int x_access = i * (width / (float)lenghtJ);

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (width * height * component);
			inTopLine[component][i] = compInPtr[x_access + y * width];
			inBotLine[component][i] = compInPtr[x_access + (height - 1 - y) * width];
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

void frameData::lineData::interpLinesCompression(void) {

	if (parent.customInterp.func != nullptr) {
		for (int i = 0; i < lenghtJ; i++) {
			for (int component = 0; component < 3; component++) {
				parent.customInterp.func(true, width, lenghtJ, i, inTopLine[component], outTopLine[component]);
				parent.customInterp.func(true, width, lenghtJ, i, inBotLine[component], outBotLine[component]);
			}
		}
	} else {
		for (int i = 0; i < lenghtJ; i++) {

			float sumTop[3] = { 0.5f, 0.5f, 0.5f },
				sumBot[3] = { 0.5f, 0.5f, 0.5f };

			for (int j = 0; j < taps; j++) {
				float coeff = coeffs[j][i];
				int x_access = i + j - tapsOffset;
				x_access += (x_access < 0) * lenghtJ;
				x_access -= (x_access >= lenghtJ) * lenghtJ;
				if ((x_access < 0) || (x_access >= lenghtJ)) x_access = x_access % lenghtJ;
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
	for (int i = 0; i < lenghtJ; ++i) {
		int x_access = xIndexes[i];
		int y_access = yIndexes[i];
		for (int component = 0; component < 3; component++) {
			T* compOutPtr = out + ((width / 2) * height * component);
			compOutPtr[x_access + y_access * width / 2] = std::max(std::min(outTopLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
			compOutPtr[x_access + (height - 1 - y_access) * width / 2] = std::max(std::min(outBotLine[component][i], (1 << parent.bitPerSubPixel) - 1), 0);
		}
	}
}

void frameData::lineData::constructLUT(void) {

	const int halfLenghtJ = lenghtJ / 2;
	const int halfHeight = height / 2;
	const int x_inner_offset = halfHeight - 2 * y - 1;
	const int x_left_offset = halfHeight - y - 1;
	const int x_right_offset = halfHeight + y;

	for (int x = 0; x < lenghtJ; x++) {
		int temp = abs(halfLenghtJ - 0.5 - x);
		if (y > temp) {
			yIndexes[x] = y;
			xIndexes[x] = x + x_inner_offset;
		} else {
			yIndexes[x] = halfLenghtJ - temp - 1;
			xIndexes[x] = (x < halfLenghtJ) ? x_left_offset : x_right_offset;
		}
	}

	if (!op) {
		const double distanceJ = lenghtJ / (double)width;
		for (int x = 0; x < width / 2; x++)
			lineIndexes[x] = std::floor(distanceJ * x);
	}

	if (parent.interp.func)
		op ? buildCompressLineCoeffs() : buildDecompressLineCoeffs();

}
