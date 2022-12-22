#pragma once
#define __AVX512F__
#include <immintrin.h>
#undef __AVX512F__

#include <cstdint>
#include <climits>
#include <chrono>
#include <vector>
#include <array>

#include "vectormath_hyp.h"
#include "vectormath_trig.h"

#include "ThreadExecutor.h"

template <typename T, typename U, bool NT>
struct frameData : public ThreadedExecutor<T> {

	using DataType = T;

	frameData(int dim, int taps, int numThreads = 1);

	frameData() = delete;

	virtual ~frameData();

	std::vector<float> buildCoeffs(double x);

	virtual void kernel(const int id);

	static void expandUV(T* data, int dim);

protected:

	struct lineData;

	const int dim;
	const int taps;
	std::vector<lineData> lines;
};

template <typename T, typename U, bool NT>
frameData<T, U, NT>::frameData(int dim, int taps, int numThreads) : ThreadedExecutor<T>(dim, numThreads), dim(dim), taps(taps) {
	lines.reserve(dim / 2);
	for (int i = 0; i < dim / 2; i++)
		lines.emplace_back(*this, i, dim);
}

template <typename T, typename U, bool NT>
frameData<T, U, NT>::~frameData() { 
	this->stop();
	lines.clear(); 
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::expandUV(T* data, int dim) {

	struct wrapper {
		T* data;
		size_t dim;
		wrapper(T* data, const size_t dim) : data(data), dim(dim) {};
		T& at(const size_t x, const size_t y, const size_t comp) { return data[x + y * dim + (dim * dim * comp)]; };
	};

	auto temp = new T[dim * dim / 2];
	std::memcpy(temp, data, dim * dim / 2);
	wrapper input(temp, dim / 2);
	wrapper output(data, dim);

	const Vec32uc LUT(0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15);

	for (int y = 0; y < dim / 2; y++) {
		int x = 0;
		for (; x < (dim / 2) - (Vec16uc().size() - 1); x += Vec16uc().size()) {
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

template <typename T, typename U, bool NT>
std::vector<float> frameData<T, U, NT>::buildCoeffs(double x) {
	std::vector<float> ret(taps, 0.0f);
	ret[-(taps / 2 - taps + 1)] = 1.0f;
	return ret;
}

template <typename T, typename U, bool NT>
void frameData<T, U, NT>::kernel(const int id) {
	for (size_t i = id; i < dim / 2; i += this->numThreads) // topo e fundo por iteração
		lines[i].processLine(this->input, this->output);
};

#include "lineData.h"