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
	
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp, int numThread);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interpFunc_t interp);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, int numThreads);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits);

	frameData() = delete;

	virtual ~frameData();

	template<typename T>
	static void expandUV(T* data, int width, int height);

	void buildFrameCoeffs(void);

	virtual void kernel(const int id);

	virtual std::vector<float> coeffsFunc(double x);

	const bitPerSubPixel_t bitPerSubPixel;

	struct lineData {

		lineData(frameData& parent, int y);

		lineData() = delete;

		virtual ~lineData();

		virtual void decompressLine(const void* in, void* out);

		virtual void compressLine(const void* in, void* out);

		void buildDecompressLineCoeffs(void);

		void buildCompressLineCoeffs(void);

	protected:
		template<typename T>
		void gatherLinesDecompression(const T* in);

		virtual void interpLinesDecompression(void);

		template<typename T>
		void storeLinesDecompression(T* out);

		template<typename T>
		void gatherLinesCompression(const T* in);

		virtual void interpLinesCompression(void);

		template<typename T>
		void storeLinesCompression(T* out);

		virtual void constructGatherLUT(void);

		virtual void constructScatterLUT(void);

#if INSTRSET >= 8 // AVX2
		template<typename T>
		void store2out(const int* in, T* out, int length);
#endif

	public:
		const bool op;
		const int len;
		const int width;
		const int height;

	protected:
		const int y;
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

	const bool op;
	const int taps;
	const int width;
	const int height;
	const interpFunc_t interp;
	std::vector<lineData> lines;
};
