#pragma once

#include <cmath>
#include <numbers>
#include <vector>
#include <string>

struct lineData;

struct interp_t {
	std::vector<float>(*func)(double x, int taps);
	int taps;
	interp_t() = delete;
	explicit interp_t(void* ptr) { (void)ptr; func = nullptr; taps = 0; };
	interp_t(std::vector<float>(*func)(double x, int taps), int taps, const char* name) : func(func), taps(taps), name(name) {};
	const std::string name;
};

struct customInterp_t {
	void (*func)(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);
	int taps;
	explicit customInterp_t(void* ptr) { (void)ptr; func = nullptr; taps = 0; };
	customInterp_t(void (*func)(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out), int taps, const char* name) : func(func), taps(taps), name(name) {};
	const std::string name;
};


std::vector<float> nearest(double x, int taps);
std::vector<float> linear(double x, int taps);
std::vector<float> cubic(double x, int taps);
std::vector<float> catmull_rom(double x, int taps);
std::vector<float> lanczos2(double x, int taps);
std::vector<float> lanczos3(double x, int taps);
std::vector<float> lanczos4(double x, int taps);
std::vector<float> lanczosN(double x, int taps);

void centrip_catmull_rom(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);

#define create_interp(s, x) { s, x, #s }

inline interp_t interpolators[] = {
   create_interp(nearest, 1),
   create_interp(linear, 2),
   create_interp(cubic, 4),
   create_interp(catmull_rom, 4),
   create_interp(lanczos2, 4),
   create_interp(lanczos3, 6),
   create_interp(lanczos4, 8),
   create_interp(lanczosN, -1)
};

inline customInterp_t customInterpolators[] = {
	create_interp(centrip_catmull_rom, 4),
};

#undef create_interp

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
