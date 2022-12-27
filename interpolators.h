#pragma once

#include <cmath>
#include <numbers>
#include <vector>

struct lineData;

struct interp_t {
	std::vector<float>(*func)(double x, int taps);
	int taps;
	interp_t(void* ptr) { (void)ptr;  };
	interp_t(std::vector<float>(*func)(double x, int taps), int taps) : func(func), taps(taps) {};
};

struct customInterp_t {
	void (*func)(const int width, const int len, const int i, const float* __restrict in, int* __restrict out);
	int taps;
	customInterp_t(void* ptr) { (void)ptr;};
	customInterp_t(void (*func)(const int width, const int len, const int i, const float* __restrict in, int* __restrict out), int taps) : func(func), taps(taps) {};
};

extern interp_t interpolators[];
extern customInterp_t customInterpolators[];

std::vector<float> nearest(double x, int taps);
std::vector<float> linear(double x, int taps);
std::vector<float> cubic(double x, int taps);
std::vector<float> lanczos2(double x, int taps);
std::vector<float> lanczos3(double x, int taps);
std::vector<float> lanczos4(double x, int taps);
std::vector<float> lanczosN(double x, int taps);

void centripetalCatMullRomInterpolation(const int width, const int len, const int i, const float* __restrict in, int* __restrict out);
void centripetalCatMullRomInterpolation_AVX2(const int width, const int len, const int i, const float* __restrict in, int* __restrict out);

enum interpolator {
	NEAREST = 0,
	LINEAR,
	CUBIC,
	LANCZOS2,
	LANCZOS3,
	LANCZOS4,
	LANCZOSN
};

enum customInterpolator {
	CENTRIPETAL_CATMULL_ROM = interpolator::LANCZOSN + 1,
	CENTRIPETAL_CATMULL_ROM_AVX2
};


