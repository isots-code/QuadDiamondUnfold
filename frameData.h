#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include <cstdint>
#include <climits>
#include <chrono>
#include <vector>
#include <array>

#include "instrset.h"

#if INSTRSET >= 8 // AVX2
#include "vectormath_hyp.h"
#include "vectormath_trig.h"
#endif

#include "ThreadExecutor.h"

struct frameData : public ThreadedExecutor {

	enum bitPerSubPixel_t {
		BITS_8 = 8,
		BITS_9,
		BITS_10,
		BITS_11,
		BITS_12,
		BITS_13,
		BITS_14,
		BITS_15,
		BITS_16
	};

	struct lineData;

public:
	typedef void (*interpFunc_t)(frameData::lineData& self, const int x, const float* __restrict in, int* __restrict out);
	
	frameData(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThread);
	frameData(int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp);
	frameData(int dim, int taps, bitPerSubPixel_t bits, int numThreads);
	frameData(int dim, int taps, bitPerSubPixel_t bits);

	frameData() = delete;

	virtual ~frameData();

	template<typename T>
	static void expandUV(T* data, int dim);

	void buildFrameCoeffs(void);

	virtual void kernel(const int id);

	virtual std::vector<float> coeffsFunc(double x);

	const bitPerSubPixel_t bitPerSubPixel;

	struct lineData {

		lineData(frameData& parent, int y, int height);

		lineData() = delete;

		virtual ~lineData();

		virtual void processLine(const void* in, void* out);

		virtual void buildLineCoeffs(void);

	protected:
		template<typename T>
		void gatherLines(const T* in);

		virtual void interpLines(void);

		template<typename T>
		void storeLines(T* out);

		virtual void constructGatherLUT(void);

#if INSTRSET >= 8 // AVX2
		template<typename T>
		void store2out(const int* in, T* out, int length);
#endif

	public:
		const int len;
		const int width;
		const int height;

	protected:
		const int y;
		const int dim;
		const int taps;
		const int linePad;
		const int paddedLen;
		const int tapsOffset;
		const size_t outTopOffset;
		const size_t outBotOffset;
		std::array<int*, 3> outTopLine;
		std::array<int*, 3> outBotLine;
		std::array<float*, 3> inTopLine;
		std::array<float*, 3> inBotLine;
		std::vector<std::vector<float>> coeffs;
		std::vector<uint16_t> xIndexes;
		std::vector<uint16_t> yIndexes;
		std::vector<uint16_t> lineIndexes;
		frameData& parent;

	};

	const int dim;
	const int taps;
	const interpFunc_t interp;
	std::vector<lineData> lines;
};
