#pragma once

#include <vector>
#include <array>

#include "extras.h"

template <typename T, typename U, bool NT>
struct frameData;

template <typename T, typename U, bool NT>
struct frameData<T, U, NT>::lineData {

	lineData(frameData& parent, int y, int width);

	lineData() = delete;

	~lineData();

	virtual void processLine(const T* in, T* out);

protected:
	void gatherLines(const T* in);

	virtual void interpLines(void);

	void storeLines(T* out);

	void constructGatherLUT(void);

public:
	const int len;
	const int width;

protected:
	const int y;
	const int dim;
	const int taps;
	const int linePad;
	const int paddedLen;
	const int tapsOffset;
	const size_t outTopOffset;
	const size_t outBotOffset;
	std::array<U*, 3> outTopLine;
	std::array<U*, 3> outBotLine;
	std::array<float*, 3> inTopLine;
	std::array<float*, 3> inBotLine;
	std::vector<std::vector<float>> coeffs;
	std::vector<uint16_t> xIndexes;
	std::vector<uint16_t> yIndexes;
	std::vector<uint16_t> lineIndexes;
	frameData& parent;

};

template <typename T, typename U, bool NT>
frameData<T, U, NT>::lineData::lineData(frameData& parent, int y, int width)
	: len(y * 4 + 2), width(width), y(y), dim(parent.dim), taps(parent.taps), linePad(Vec8i().size()), paddedLen((len / linePad)* linePad + linePad),
	tapsOffset(-(taps / 2 - taps + 1)), outTopOffset(y* (width * 2)), outBotOffset((width - 1 - y)* (width * 2)), parent(parent) {
	xIndexes.resize(paddedLen);
	yIndexes.resize(paddedLen);
	lineIndexes.resize(width);
	coeffs.resize(taps);
	for (auto& subCoeffs : coeffs)
		subCoeffs.resize(width);
	constructGatherLUT();
}

template <typename T, typename U, bool NT>
frameData<T, U, NT>::lineData::~lineData() {
	xIndexes.clear();
	yIndexes.clear();
	lineIndexes.clear();
	for (auto& subCoeffs : coeffs)
		subCoeffs.clear();
	coeffs.clear();
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::lineData::processLine(const T* in, T* out) {
	float inTopArray[3][paddedLen + taps];
	float inBotArray[3][paddedLen + taps];
	U outTopArray[3][width * 2];
	U outBotArray[3][width * 2];
	inTopLine = { inTopArray[0] + tapsOffset, inTopArray[1] + tapsOffset, inTopArray[2] + tapsOffset };
	inBotLine = { inBotArray[0] + tapsOffset, inBotArray[1] + tapsOffset, inBotArray[2] + tapsOffset };
	outTopLine = { outTopArray[0] + tapsOffset, outTopArray[1] + tapsOffset, outTopArray[2] + tapsOffset };
	outBotLine = { outBotArray[0] + tapsOffset, outBotArray[1] + tapsOffset, outBotArray[2] + tapsOffset };
	gatherLines(in);
	interpLines();
	storeLines(out);
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::lineData::gatherLines(const T* in) {
	for (unsigned long long i = 0; i < xIndexes.size(); i += Vec8us().size()) {

		Vec8i x = extend(Vec8us().load(&(xIndexes[i])));
		Vec8i y = extend(Vec8us().load(&(yIndexes[i])));

		for (int component = 0; component < 3; component++) {
			auto compInPtr = in + (width * width * component);
			gather(compInPtr, x + y * width).store(&(inTopLine[component][i]));
			gather(compInPtr, x + (width - 1 - y) * width).store(&(inBotLine[component][i]));
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

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::lineData::interpLines(void) {

	const int Lj = len / 2;

	for (int i = 0; i < width; i += Vec8f().size()) {

		Vec8f sumTR[3] = { Vec8f(0), Vec8f(0) , Vec8f(0) },
			sumTL[3] = { Vec8f(0), Vec8f(0) , Vec8f(0) },
			sumBR[3] = { Vec8f(0), Vec8f(0) , Vec8f(0) },
			sumBL[3] = { Vec8f(0), Vec8f(0) , Vec8f(0) };
		Vec8i indexes = extend(Vec8us().load(&(lineIndexes[i])));
		Vec8i baseIndex(lineIndexes[i]);

#pragma unroll(1)
		for (int j = 0; j < taps; j++) {
			Vec8i tempIndexes = indexes + j - tapsOffset;
			Vec8f coeff = Vec8f().load(&coeffs[j][i]);
			for (int component = 0; component < 3; component++) {
				sumTL[component] += lookup8(tempIndexes - baseIndex, Vec8f().load(inTopLine[component] + lineIndexes[i])) * coeff;
				sumTR[component] += lookup8(tempIndexes - baseIndex, Vec8f().load(inTopLine[component] + lineIndexes[i] + Lj)) * coeff;
				sumBL[component] += lookup8(tempIndexes - baseIndex, Vec8f().load(inBotLine[component] + lineIndexes[i])) * coeff;
				sumBR[component] += lookup8(tempIndexes - baseIndex, Vec8f().load(inBotLine[component] + lineIndexes[i] + Lj)) * coeff;
			}
		}

		for (int component = 0; component < 3; component++) {
			if constexpr (std::is_same_v<U, float>) {
				(sumTL[component]).store(&(outTopLine[component][i]));
				(sumTR[component]).store(&(outTopLine[component][i + width]));
				(sumBL[component]).store(&(outBotLine[component][i]));
				(sumBR[component]).store(&(outBotLine[component][i + width]));
			} else {
				roundi(sumTL[component] + 0.5f).store(&(outTopLine[component][i]));
				roundi(sumTR[component] + 0.5f).store(&(outTopLine[component][i + width]));
				roundi(sumBL[component] + 0.5f).store(&(outBotLine[component][i]));
				roundi(sumBR[component] + 0.5f).store(&(outBotLine[component][i + width]));
			}
		}
	}
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::lineData::storeLines(T* out) {
	for (int component = 0; component < 3; component++) {
		auto compOutPtr = out + (width * width * 2 * component);
		store2out<NT>(outTopLine[component], compOutPtr + outTopOffset, width * 2);
		store2out<NT>(outBotLine[component], compOutPtr + outBotOffset, width * 2);
	}
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::lineData::constructGatherLUT(void) {

	const int Lj = len / 2;
	const int halfWidth = width / 2;
	const int x_inner_offset = halfWidth - 2 * y - 1;
	const int x_left_offset = halfWidth - y - 1;
	const int x_right_offset = halfWidth + y;

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

	const double Dj = Lj / (double)width;
	for (int x = 0; x < width; x++)
		lineIndexes[x] = floor(Dj * x);

	for (int x = 0; x < width; x++) {
		auto x_ = Dj * x;
		x_ -= floor(x_);
		auto coeff = parent.buildCoeffs(x_);
		for (int i = 0; i < taps; i++)
			coeffs[i][x] = coeff[i];
	}
}