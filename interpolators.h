#pragma once

#include <cmath>
#include <numbers>
#include <vector>

struct lineData;

struct interp_t {
	std::vector<float>(*func)(double x, int taps);
	int taps;
	interp_t(void* ptr) { (void)ptr; func = nullptr; taps = 0; };
	interp_t(std::vector<float>(*func)(double x, int taps), int taps) : func(func), taps(taps) {};
};

struct customInterp_t {
	void (*func)(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);
	int taps;
	customInterp_t(void* ptr) { (void)ptr; func = nullptr; taps = 0; };
	customInterp_t(void (*func)(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out), int taps) : func(func), taps(taps) {};
};

std::vector<float> nearest(double x, int taps);
std::vector<float> linear(double x, int taps);
std::vector<float> cubic(double x, int taps);
std::vector<float> catmull_rom(double x, int taps);
std::vector<float> lanczos2(double x, int taps);
std::vector<float> lanczos3(double x, int taps);
std::vector<float> lanczos4(double x, int taps);
std::vector<float> lanczosN(double x, int taps);

void centripetalCatMullRomInterpolation(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);

inline interp_t interpolators[] = {
   { nearest, 1 },
   { linear, 2 },
   { cubic, 4 },
   { catmull_rom, 4 },
   { lanczos2, 4 },
   { lanczos3, 6 },
   { lanczos4, 8 },
   { lanczosN, -1 }
};

inline customInterp_t customInterpolators[] = {
	{ centripetalCatMullRomInterpolation, 4},
};

enum interpolator {
	NEAREST = 0,
	LINEAR,
	CUBIC,
	CATMULL_ROM,
	LANCZOS2,
	LANCZOS3,
	LANCZOS4,
	LANCZOSN,
	CENTRIPETAL_CATMULL_ROM = 0
};
