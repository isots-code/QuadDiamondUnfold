#include "interpolators.h"

#include "instrset.h"

void centripetalCatMullRomInterpolation_scalar(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);
extern void centripetalCatMullRomInterpolation_AVX2(const bool op, const int width, const int len, const int i, const float* __restrict in, int* __restrict out);

std::vector<float> nearest(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);
	x = std::roundf((float)x);
	ret[0] = 1 - x;
	ret[1] = x;
	return ret;
}

std::vector<float> linear(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);
	ret[0] = 1 - x;
	ret[1] = x;
	return ret;
}

std::vector<float> cubic(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);
	auto x2 = x * x;
	ret[0] = -x * (x * (x - 2.0) + 1.0);
	ret[1] = x2 * (x - 2.0) + 1.0;
	ret[2] = -x * (x * (x - 1.0) - 1.0);
	ret[3] = x2 * (x - 1.0);
	return ret;
}

std::vector<float> catmull_rom(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);
	auto x2 = x * x;
	auto x3 = x2 * x;
	ret[3] = (x3 - x2) / 2;
	ret[2] = (-3 * x3 + 4 * x2 + x) / 2;
	ret[1] = (3 * x3 - 5 * x2 + 2) / 2;
	ret[0] = (-x3 + 2 * x2 - x) / 2;
	return ret;
}

std::vector<float> lanczos2(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);

	if (x == 0.0) {
		ret[-(taps / 2 - taps + 1)] = 1.0;
		return ret;
	}

	auto deg2rad = [](double degrees) -> double {
		return degrees * std::numbers::pi / 180;
	};

	auto sum = 0.0;
	for (unsigned int i = 0; i < ret.size(); i++)
		sum += ret[i] = (std::cos(deg2rad(i * -90.0)) * std::sin((-x - 1.0) * std::numbers::pi / 2.0) +
			std::sin(deg2rad(i * -90.0)) * std::cos((-x - 1.0) * std::numbers::pi / 2.0))
		/ std::pow(i - 1.0 - x, 2.0);

	for (unsigned int i = 0; i < ret.size(); i++)
		ret[i] /= sum;

	return ret;
}

std::vector<float> lanczos3(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);

	if (x == 0.0) {
		ret[-(taps / 2 - taps + 1)] = 1.0;
		return ret;
	}

	auto deg2rad = [](double degrees) -> double {
		return degrees * std::numbers::pi / 180.0;
	};

	auto sum = 0.0;
	for (unsigned int i = 0; i < ret.size(); i++)
		sum += ret[i] = (std::cos(deg2rad(i * -120.0)) * std::sin((-x - 2.0) * std::numbers::pi / 3.0) +
			std::sin(deg2rad(i * -120.0)) * std::cos((-x - 2.0) * std::numbers::pi / 3.0))
		/ std::pow(i - 2.0 - x, 2.0);

	for (unsigned int i = 0; i < ret.size(); i++)
		ret[i] /= sum;

	return ret;
}

std::vector<float> lanczos4(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);

	if (x == 0.0) {
		ret[-(taps / 2 - taps + 1)] = 1.0;
		return ret;
	}

	auto deg2rad = [](double degrees) -> double {
		return degrees * std::numbers::pi / 180.0;
	};

	auto sum = 0.0;
	for (unsigned int i = 0; i < ret.size(); i++)
		sum += ret[i] = (std::cos(deg2rad(i * -135.0)) * std::sin((-x - 3.0) * std::numbers::pi / 4.0) +
			std::sin(deg2rad(i * -135.0)) * std::cos((-x - 3.0) * std::numbers::pi / 4.0))
		/ std::pow(i - 3.0 - x, 2.0);

	for (unsigned int i = 0; i < ret.size(); i++)
		ret[i] /= sum;

	return ret;
}

std::vector<float> lanczosN(double x, int taps) {
	auto ret = std::vector<float>(taps, 0.0);

	if (x == 0.0) {
		ret[-(taps / 2 - taps + 1)] = 1.0;
		return ret;
	}

	auto deg2rad = [](double degrees) -> double {
		return degrees * std::numbers::pi / 180.0;
	};

	auto halfTaps = taps / 2.0;
	auto angle = (360.0 / (2.0 * halfTaps)) * (halfTaps - 1.0);

	auto sum = 0.0;
	for (unsigned int i = 0; i < ret.size(); i++)
		sum += ret[i] = (std::cos(deg2rad(i * -angle)) * std::sin((-x - halfTaps + 1.0) * std::numbers::pi / (taps / 2)) +
			std::sin(deg2rad(i * -angle)) * std::cos((-x - halfTaps + 1.0) * std::numbers::pi / (taps / 2)))
		/ std::pow(i - halfTaps + 1.0 - x, 2.0);

	for (unsigned int i = 0; i < ret.size(); i++)
		ret[i] /= sum;

	return ret;
}

void centrip_catmull_rom(const bool op, const int width, const int lenghtJ, const int i, const float* __restrict in, int* __restrict out) {
	if (instrset_detect() >= 8)
		centripetalCatMullRomInterpolation_AVX2(op, width, lenghtJ, i, in, out);
	else
		centripetalCatMullRomInterpolation_scalar(op, width, lenghtJ, i, in, out);
}

void centripetalCatMullRomInterpolation_scalar(const bool op, const int width, const int lenghtJ, const int i, const float* __restrict in, int* __restrict out) {
	const float Dj = op ? (float)width / lenghtJ : lenghtJ / (float)width;
	float x = Dj * i;
	float x_floor = std::floorf(x);
	x -= x_floor;
	int x_int = x_floor;

	float x0 = in[x_int - 1];
	float x1 = in[x_int + 0];
	float x2 = in[x_int + 1];
	float x3 = in[x_int + 2];

	float t01 = std::powf((x1 - x0) * (x1 - x0) + 1.0f, 0.25f);
	float t12 = std::powf((x2 - x1) * (x2 - x1) + 1.0f, 0.25f);
	float t23 = std::powf((x3 - x2) * (x3 - x2) + 1.0f, 0.25f);
	float m1 = (x2 - x1 + t12 * ((x1 - x0) / t01 - (x2 - x0) / (t01 + t12)));
	float m2 = (x2 - x1 + t12 * ((x3 - x2) / t23 - (x3 - x1) / (t12 + t23)));
	float res = (((2.0f * (x1 - x2) + m1 + m2) * x + (-3.0f * (x1 - x2) - m1 - m1 - m2)) * x + m1) * x + x1;

	out[i] = std::roundf(res);
}