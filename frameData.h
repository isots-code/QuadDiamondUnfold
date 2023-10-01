#pragma once

#include <vector>
#include <array>

#include "interpolators.h"

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

	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, customInterp_t interp, bool simd, int numThread);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, customInterp_t interp, int numThread);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, customInterp_t interp);
	frameData(bool op, int dim, bitPerSubPixel_t bits, customInterp_t interp, bool simd, int numThread);
	frameData(bool op, int dim, bitPerSubPixel_t bits, customInterp_t interp, int numThread);
	frameData(bool op, int dim, bitPerSubPixel_t bits, customInterp_t interp);

	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interp_t interp, bool simd, int numThread);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interp_t interp, int numThread);
	frameData(bool op, int dim, int taps, bitPerSubPixel_t bits, interp_t interp);
	frameData(bool op, int dim, bitPerSubPixel_t bits, interp_t interp, bool simd, int numThread);
	frameData(bool op, int dim, bitPerSubPixel_t bits, interp_t interp, int numThread);
	frameData(bool op, int dim, bitPerSubPixel_t bits, interp_t interp);
	frameData(bool op, int dim, bitPerSubPixel_t bits, int numThreads);
	frameData(bool op, int dim, bitPerSubPixel_t bits);

	frameData() = delete;

	virtual ~frameData();

	template<typename T>
	static void expandUV(T* data, int width, int height);

	template<typename T>
	static void expandUV_AVX2(T* data, int width, int height);

	void kernel(const int id) override;

	const bitPerSubPixel_t bitPerSubPixel;

	struct lineData {

		lineData(frameData& parent, int y);

		lineData() = delete;

		~lineData();

		void decompressLine(const void* in, void* out);

		void compressLine(const void* in, void* out);

		void buildDecompressLineCoeffs(void);

		void buildCompressLineCoeffs(void);

	protected:
		template<typename T>
		void gatherLinesDecompression(const T* in);

		void interpLinesDecompression(void);

		template<typename T>
		void storeLinesDecompression(T* out);

		template<typename T>
		void gatherLinesDecompression_AVX2(const T* in);

		void interpLinesDecompression_AVX2(void);

		template<typename T>
		void storeLinesDecompression_AVX2(T* out);

		template<typename T>
		void gatherLinesCompression(const T* in);

		void interpLinesCompression(void);

		template<typename T>
		void storeLinesCompression(T* out);

		template<typename T>
		void gatherLinesCompression_AVX2(const T* in);

		void interpLinesCompression_AVX2(void);

		template<typename T>
		void storeLinesCompression_AVX2(T* out);

		void constructLUT(void);

	public:
		const bool op;
		const int lenghtJ;
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
	const bool simd;
	const int taps;
	const int width;
	const int height;
	const interp_t interp;
	const customInterp_t customInterp;
	std::vector<lineData> lines;
};
